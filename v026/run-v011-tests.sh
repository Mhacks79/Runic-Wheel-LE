#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
python "$here/Regression09Tests.py"
python "$here/ControlParityTests.py"
python "$here/UITaskContractTests.py"
python "$here/SpellLayoutTests.py"
python "$here/SpellNativeRouteTests.py"
bash "$here/run-host-tests.sh"
python "$here/NativeContractTests.py"
for opt in -O2 -O1; do
  name="$(mktemp /tmp/rw-native.XXXXXX)"
  if [[ "$opt" == "-O1" ]]; then
    clang++ -std=c++17 $opt -fno-omit-frame-pointer -fsanitize=address,undefined -DNDEBUG "$here/NativeActionPolicyTests.cpp" -o "$name"
  else
    clang++ -std=c++17 $opt -DNDEBUG -Wall -Wextra -Werror "$here/NativeActionPolicyTests.cpp" -o "$name"
  fi
  "$name"; rm -f "$name"
done
for name in GestureParityTests TimePolicyTests UITaskPolicyTests ContextPolicyTests MenuTrackerTests SpellMembershipTests; do
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
