#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/../../.." && pwd)"
llvm="${LLVM_MINGW_ROOT:-/mnt/data/llvm-toolchains/llvm-mingw-20261020-msvcrt-ubuntu-22.04-x86_64}"
clang="${CLANG_CL:-/usr/local/swift/usr/bin/clang-cl}"
dir="${1:-/mnt/data/runic-v05-msvcabi}"
mkdir -p "$dir"
"$clang" /nologo /c /O1 /EHa /GS- /GR- /W4 /clang:-target /clang:i686-pc-windows-msvc \
  "/I$root/sdk/skse-le-sdk" "/I$root/sdk/skse-le-sdk/skse" "/Fo$dir/RunicWheelV06.obj" "$here/RunicWheelV06.cpp"
"$llvm/bin/lld-link" /nologo /dll /noentry /machine:x86 /nodefaultlib \
  "/out:$dir/RunicWheelLE.dll" "$dir/RunicWheelV06.obj" \
  "$llvm/i686-w64-mingw32/lib/libmsvcrt.a" \
  "$llvm/i686-w64-mingw32/lib/libkernel32.a" \
  "$llvm/i686-w64-mingw32/lib/libuser32.a"
python "$here/verify-v06.py" "$dir/RunicWheelLE.dll"
echo "MSVC target x86 V0.6 variant-aware V2 co-save/editor internal artifact: $dir/RunicWheelLE.dll"
