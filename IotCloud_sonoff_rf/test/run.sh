#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
json_dir="$project_dir/.pio/libdeps/esp8285_1m/ArduinoJson/src"
if [ ! -d "$json_dir" ]; then
    echo 'Build the firmware with PlatformIO first to install ArduinoJson.' >&2
    exit 1
fi
out_dir=$(mktemp -d)
trap 'rm -rf "$out_dir"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$project_dir/test/test_protocol.cpp" -o "$out_dir/protocol"
"$out_dir/protocol"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Wno-deprecated-declarations -fsanitize=address,undefined \
    -I"$project_dir/test/fakes" -I"$json_dir" \
    "$project_dir/src/rf_bridge.cpp" "$project_dir/test/test_bridge.cpp" -o "$out_dir/bridge"
"$out_dir/bridge"
