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

# The optional target (canary, early or device IDs) makes it a staged
# rollout; see deploy_ota.sh, which also promotes it without re-uploading.
if [ "$#" -lt 3 ] || [ "$#" -gt 4 ]; then
  echo "Usage: $0 <firmware.bin> <model> <semantic-version> [canary|early|<device-id>[,<device-id>...]]" >&2
  exit 2
fi

server_alias="${OTA_SERVER_ALIAS:-csm-server}"
ota_bin_path="$(realpath "$1")"
model_num="$2"
version="$3"
parent_path="IotCloud2/ota/data/$model_num"
bin_path="$parent_path/$version"

if [[ ! "$model_num" =~ ^[A-Za-z0-9][A-Za-z0-9._-]{0,99}$ ]]; then
  echo "Error: model must contain only letters, numbers, '.', '_' or '-'" >&2
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

# Arguments after the binary are passed on as given: model, version, target.
bash "$SCRIPT_DIR/deploy_ota.sh" "${@:2}" || {
  echo "The firmware is uploaded; rerun tools/deploy_ota.sh ${*:2} to deploy it." >&2
  exit 1
}
