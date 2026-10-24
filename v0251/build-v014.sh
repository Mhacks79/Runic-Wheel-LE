#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../../.." && pwd)"
llvm="${LLVM_MINGW_ROOT:-/mnt/data/toolchain/llvm-mingw-20261020-msvcrt-ubuntu-22.04-x86_64}"
clang="${CLANG_CL:-/usr/local/swift/usr/bin/clang-cl}"
out="${1:-/mnt/data/rw-v012-build}"
export TESV_EXE="${TESV_EXE:-$root/inputs/TESV.exe}"
python "$here/ProfessionalArtTests.py"
python "$here/DisplayNameContractTests.py"
python "$here/qa/pro_art_qa.py"
python "$here/InventoryCountContractTests.py"
python "$here/ControlParityTests.py"
python "$here/Regression09Tests.py"
python "$here/UITaskContractTests.py"
python "$here/SpellLayoutTests.py"
python "$here/SpellNativeRouteTests.py"
python "$root/projects/RunicWheelLE/quality/validate.py" --profile dev --source "$here/RunicWheelV011.cpp" --ini "$here/RunicWheelLE.ini"
mkdir -p "$out"
"$clang" /nologo /c /O1 /EHa /GS- /GR- /W4 /clang:-target /clang:i686-pc-windows-msvc \
  "/I$root/sdk/skse-le-sdk" "/I$root/sdk/skse-le-sdk/skse" \
  "/Fo$out/RunicWheelLE.obj" "$here/RunicWheelV011.cpp"
"$llvm/bin/lld-link" /nologo /dll /noentry /machine:x86 /nodefaultlib \
  "/out:$out/RunicWheelLE.dll" "$out/RunicWheelLE.obj" \
  "$llvm/i686-w64-mingw32/lib/libmsvcrt.a" \
  "$llvm/i686-w64-mingw32/lib/libkernel32.a" \
  "$llvm/i686-w64-mingw32/lib/libuser32.a"
LLVM_MINGW_ROOT="$llvm" python "$here/verify-v011.py" "$out/RunicWheelLE.dll"
echo "PASS: $out/RunicWheelLE.dll (PRIVATE v0.14 PROFESSIONAL ART PRIVATE, game-validation pending)"
