#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
for name in BindingsAndActionsTests PaletteEditorTests; do
  "${CXX:-c++}" -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Werror "$here/$name.cpp" -o "$tmp/$name-release"
  "$tmp/$name-release"
  "${SAN_CXX:-clang++}" -std=c++17 -O1 -g -fno-omit-frame-pointer -Wall -Wextra -Werror -fsanitize=address,undefined "$here/$name.cpp" -o "$tmp/$name-sanitized"
  ASAN_OPTIONS=detect_leaks=1 "$tmp/$name-sanitized"
done
echo 'V0.6 Release and sanitizers: 4/4 PASS'
