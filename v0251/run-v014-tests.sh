#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
python "$here/ProfessionalArtTests.py"
python "$here/DisplayNameContractTests.py"
python "$here/qa/pro_art_qa.py"
python "$here/InventoryCountContractTests.py"
python "$here/ControlParityTests.py"
python "$here/UITaskContractTests.py"
python "$here/SpellNativeRouteTests.py"
for opt in -O2 -O1; do
  flags=(-std=c++17 "$opt" -Wall -Wextra -Werror)
  if [[ "$opt" == '-O1' ]]; then flags+=(-fno-omit-frame-pointer -fsanitize=address,undefined); fi
  for file in DisplayNameTests IconPolicyTests InventoryCountTests; do
    bin="$(mktemp /tmp/runic-pro-XXXXXXXX)"
    trap 'rm -f "$bin"' EXIT
    clang++ "${flags[@]}" "$here/${file}.cpp" -o "$bin"
    "$bin"
    rm -f "$bin"
    trap - EXIT
  done
done
bash "$here/run-v011-tests.sh"
