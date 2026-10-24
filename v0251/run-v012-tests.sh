#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
python "$here/DisplayNameContractTests.py"
python "$here/InventoryCountContractTests.py"
for opt in -O2 -O1;do
  out="$(mktemp /tmp/runic-labels.XXXXXXXX)"
  trap 'rm -f "$out"' EXIT
  args=(-std=c++17 "$opt" -Wall -Wextra -Werror)
  if [[ "$opt" == -O1 ]]; then args+=(-fno-omit-frame-pointer -fsanitize=address,undefined);fi
  clang++ "${args[@]}" "$here/DisplayNameTests.cpp" -o "$out"
  "$out"
  rm -f "$out"
  trap - EXIT
done
for opt in -O2 -O1;do
  bin="$(mktemp /tmp/runic-inventory.XXXXXXXX)"
  args=(-std=c++17 "$opt" -Wall -Wextra -Werror)
  if [[ "$opt" == -O1 ]];then args+=(-fno-omit-frame-pointer -fsanitize=address,undefined);fi
  clang++ "${args[@]}" "$here/InventoryCountTests.cpp" -o "$bin"
  "$bin"
  rm -f "$bin"
done
bash "$here/run-v011-tests.sh"
