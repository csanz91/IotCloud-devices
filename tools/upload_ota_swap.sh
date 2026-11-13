#!/usr/bin/env bash
set -euo pipefail

# Deploy a firmware under an EXISTING model+version slot without losing the
# image that is already there, so devices that are already on that version
# ignore the update (the device only compares version strings) and the others
# install this image.
#
#   upload_ota_swap.sh deploy  <firmware.bin> <model> <version>
#   upload_ota_swap.sh restore <model> <version>
#
# deploy:  keeps the current server file as <version>.orig, then runs
#          upload_ota.sh. Refuses if a .orig already exists (a previous swap
#          was not restored) or if there is nothing to back up.
# restore: puts <version>.orig back. Run it once the target device has
#          updated, so nobody else is ever served the swapped image.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="$SCRIPT_DIR/.env"
if [ -f "$ENV_FILE" ]; then
  set -a
  source "$ENV_FILE"
  set +a
fi

server_alias="${OTA_SERVER_ALIAS:-csm-server}"
name_regex='^[A-Za-z0-9][A-Za-z0-9._-]{0,99}$'

usage() {
  echo "Usage: $0 deploy <firmware.bin> <model> <version>" >&2
  echo "       $0 restore <model> <version>" >&2
  exit 2
}

[ "$#" -ge 1 ] || usage
action="$1"

case "$action" in
  deploy)
    [ "$#" -eq 4 ] || usage
    model_num="$3"
    version="$4"
    ;;
  restore)
    [ "$#" -eq 3 ] || usage
    model_num="$2"
    version="$3"
    ;;
  *) usage ;;
esac

for value in "$model_num" "$version"; do
  if [[ ! "$value" =~ $name_regex ]]; then
    echo "Error: invalid model or version: $value" >&2
    exit 2
  fi
done

bin_path="IotCloud/ota/data/$model_num/$version"
orig_path="$bin_path.orig"

if [ "$action" = "deploy" ]; then
  if ssh "$server_alias" "[ -e '$orig_path' ]"; then
    echo "Error: $orig_path already exists: a previous swap was not restored." >&2
    exit 1
  fi
  if ! ssh "$server_alias" "[ -f '$bin_path' ]"; then
    echo "Error: nothing at $bin_path to back up; use upload_ota.sh directly." >&2
    exit 1
  fi
  echo "Backing up the current $model_num/$version image..."
  ssh "$server_alias" "cp -p '$bin_path' '$orig_path' && sha256sum '$orig_path'"
  exec bash "$SCRIPT_DIR/upload_ota.sh" "$2" "$model_num" "$version"
fi

if ! ssh "$server_alias" "[ -f '$orig_path' ]"; then
  echo "Error: no backup at $orig_path to restore." >&2
  exit 1
fi
echo "Restoring the original $model_num/$version image..."
ssh "$server_alias" "mv -f '$orig_path' '$bin_path' && chmod 755 '$bin_path' && sha256sum '$bin_path'"
echo "Restored. Compare the hash with the one printed by deploy."
