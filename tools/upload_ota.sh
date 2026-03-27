#! /bin/bash
set -euo pipefail

# Resolve path to this script, so .env is loaded from tools/ even if cwd differs.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="$SCRIPT_DIR/.env"

if [ ! -f "$ENV_FILE" ]; then
  echo "Error: $ENV_FILE not found. Create it with MQTT_TOKEN=<token>"
  exit 1
fi

# Load .env vars (need not be exported in .env)
set -a
source "$ENV_FILE"
set +a

if [ -z "${MQTT_TOKEN:-}" ]; then
  echo "Error: MQTT_TOKEN is not set in $ENV_FILE"
  exit 1
fi

server_alias="csm-server"
ota_bin_path="$(realpath "$1")"
model_num="$2"
version="$3"
parent_path="IotCloud/ota/data/$model_num"
bin_path="$parent_path/$version"

echo "Creating directory '$parent_path' on the server..."
ssh "$server_alias" "mkdir -p $parent_path"

echo "Coping '$ota_bin_path' to the server..."
scp "$ota_bin_path" "$server_alias:$bin_path"

echo "Changing permissions of the binary file..."
ssh "$server_alias" "chmod 755 $bin_path"

echo "Requesting OTA update start"
mosquitto_pub -h mqtt.iotcloud.es -t "v1/ota/update/$model_num" -u "$MQTT_TOKEN" -P "_" -m "$version"
