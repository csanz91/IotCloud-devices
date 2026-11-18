#!/usr/bin/env bash
set -euo pipefail

# Deploy a firmware version that is already on the OTA server.
#
#   deploy_ota.sh <model> <version>                  every device of the model
#   deploy_ota.sh <model> <version> canary           devices in the canary ring
#   deploy_ota.sh <model> <version> early            canary + early rings
#   deploy_ota.sh <model> <version> <id>[,<id>...]   these devices only
#
# A ring or device list is a staged rollout: only those devices update and
# the model's release stays where it was. Run it again without a target to
# promote the version to everyone. Put a device in a ring with:
#   UPDATE iotcloud.devices SET ota_ring = 'canary' WHERE id = '<device id>';
# Staged rollouts need devices on IotCloud library 1.3.0 or newer.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="$SCRIPT_DIR/.env"

if [ -f "$ENV_FILE" ]; then
  set -a
  source "$ENV_FILE"
  set +a
fi

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
  echo "Usage: $0 <model> <semantic-version> [canary|early|<device-id>[,<device-id>...]]" >&2
  exit 2
fi

server_alias="${OTA_SERVER_ALIAS:-csm-server}"
internal_api_service="${OTA_INTERNAL_API_SERVICE:-api-internal}"
internal_api_url="${OTA_INTERNAL_API_URL:-http://localhost:8001}"
model_num="$1"
version="$2"
target="${3:-}"

if [[ ! "$model_num" =~ ^[A-Za-z0-9][A-Za-z0-9._-]{0,99}$ ]]; then
  echo "Error: model must contain only letters, numbers, '.', '_' or '-'" >&2
  exit 2
fi

if [[ ! "$internal_api_service" =~ ^[A-Za-z0-9][A-Za-z0-9._-]{0,99}$ ]]; then
  echo "Error: OTA_INTERNAL_API_SERVICE is not a valid Compose service name" >&2
  exit 2
fi

semver_regex='^v?(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(-([0-9A-Za-z-]+)(\.[0-9A-Za-z-]+)*)?(\+[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?$'
if [[ ! "$version" =~ $semver_regex ]]; then
  echo "Error: version must be semantic (for example, v1.0.10)" >&2
  exit 2
fi

# Every value below is validated, so it is safe inside the JSON body.
if [ -z "$target" ]; then
  api_path="/firmware-releases/$model_num"
  body="{\"version\":\"$version\"}"
  echo "Deploying '$version' to every '$model_num' device..."
elif [[ "$target" =~ ^(canary|early)$ ]]; then
  api_path="/firmware-releases/$model_num/rollouts"
  body="{\"version\":\"$version\",\"ring\":\"$target\"}"
  echo "Rolling out '$version' to the '$target' ring of '$model_num'..."
elif [[ "$target" =~ ^[A-Za-z0-9_-]{1,50}(,[A-Za-z0-9_-]{1,50})*$ ]]; then
  api_path="/firmware-releases/$model_num/rollouts"
  device_ids="\"${target//,/\",\"}\""
  body="{\"version\":\"$version\",\"deviceIds\":[$device_ids]}"
  echo "Rolling out '$version' to '$model_num' devices: $target..."
else
  echo "Error: target must be 'canary', 'early' or comma-separated device IDs" >&2
  exit 2
fi

# The body goes through stdin (ssh -> docker exec -i -> curl --data-binary @-)
# so it needs no shell quoting on the way.
release_response="$({
  ssh "$server_alias" \
    "attempt=0; api_container_id=''; while [ \"\$attempt\" -lt 10 ]; do api_container_id=\$(docker ps --filter 'label=com.docker.compose.service=$internal_api_service' --filter 'health=healthy' --format '{{.ID}}' | head -n 1); [ -n \"\$api_container_id\" ] && break; attempt=\$((attempt + 1)); sleep 2; done; if [ -z \"\$api_container_id\" ]; then echo 'No healthy $internal_api_service container found' >&2; exit 1; fi; docker exec -i \"\$api_container_id\" sh -c 'curl --fail-with-body --silent --show-error --request POST --header \"Content-Type: application/json\" --header \"X-M2M-Token: \$M2M_TOKEN\" --data-binary @- \"$internal_api_url$api_path\"'" \
    <<< "$body"
} 2>&1)" || {
  echo "Error: deployment failed." >&2
  echo "$release_response" >&2
  exit 1
}

echo "Accepted: $release_response"
