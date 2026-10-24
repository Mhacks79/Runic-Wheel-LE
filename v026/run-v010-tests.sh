#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
python "$here/Regression09Tests.py"
python "$here/ControlParityTests.py"
python "$here/UITaskContractTests.py"
bash "$here/run-host-tests.sh"
for name in GestureParityTests TimePolicyTests UITaskPolicyTests ContextPolicyTests MenuTrackerTests; do
  src="$here/$name.cpp"
  if [[ -f "$src" ]]; then
    release="$(mktemp /tmp/runic.XXXXXX)"
    sanitized="$(mktemp /tmp/runic.XXXXXX)"
    trap 'rm -f "$release" "$sanitized"' EXIT
    "${CXX:-clang++}" -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Werror "$src" -o "$release"
    "$release"
    "${SAN_CXX:-clang++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fno-omit-frame-pointer -fsanitize=address,undefined "$src" -o "$sanitized"
    ASAN_OPTIONS=detect_leaks=1 "$sanitized"
    rm -f "$release" "$sanitized"
    trap - EXIT
  fi
done
