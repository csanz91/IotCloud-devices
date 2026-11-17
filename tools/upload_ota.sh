#!/usr/bin/env bash
set -euo pipefail

# Resolve path to this script, so optional configuration is loaded from tools/
# even if cwd differs.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="$SCRIPT_DIR/.env"

if [ -f "$ENV_FILE" ]; then
  # Load optional overrides (need not be exported in .env).
  set -a
  source "$ENV_FILE"
  set +a
fi

if [ "$#" -ne 3 ]; then
  echo "Usage: $0 <firmware.bin> <model> <semantic-version>" >&2
  exit 2
fi

server_alias="${OTA_SERVER_ALIAS:-csm-server}"
internal_api_service="${OTA_INTERNAL_API_SERVICE:-api-internal}"
internal_api_url="${OTA_INTERNAL_API_URL:-http://localhost:8001}"
ota_bin_path="$(realpath "$1")"
model_num="$2"
version="$3"
parent_path="IotCloud2/ota/data/$model_num"
bin_path="$parent_path/$version"

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

if [ ! -f "$ota_bin_path" ]; then
  echo "Error: firmware binary not found: $ota_bin_path" >&2
  exit 2
fi

echo "Creating directory '$parent_path' on the server..."
ssh "$server_alias" "mkdir -p '$parent_path'"

echo "Copying '$ota_bin_path' to the server..."
scp "$ota_bin_path" "$server_alias:$bin_path"

echo "Changing permissions of the binary file..."
ssh "$server_alias" "chmod 755 '$bin_path'"

echo "Registering and deploying firmware release '$version' for '$model_num'..."
release_response="$({
  ssh "$server_alias" \
    "attempt=0; api_container_id=''; while [ \"\$attempt\" -lt 10 ]; do api_container_id=\$(docker ps --filter 'label=com.docker.compose.service=$internal_api_service' --filter 'health=healthy' --format '{{.ID}}' | head -n 1); [ -n \"\$api_container_id\" ] && break; attempt=\$((attempt + 1)); sleep 2; done; if [ -z \"\$api_container_id\" ]; then echo 'No healthy $internal_api_service container found' >&2; exit 1; fi; docker exec \"\$api_container_id\" sh -c 'curl --fail-with-body --silent --show-error --request POST --header \"Content-Type: application/json\" --header \"X-M2M-Token: \$M2M_TOKEN\" --data \"{\\\"version\\\":\\\"$version\\\"}\" \"$internal_api_url/firmware-releases/$model_num\"'"
} 2>&1)" || {
  echo "Error: firmware was uploaded, but release deployment failed." >&2
  echo "$release_response" >&2
  exit 1
}

echo "Release accepted: $release_response"
