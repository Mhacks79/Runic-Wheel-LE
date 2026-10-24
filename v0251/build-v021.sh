#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../../.." && pwd)"
llvm="${LLVM_MINGW_ROOT:-/mnt/data/toolchain/llvm-mingw-20261020-msvcrt-ubuntu-22.04-x86_64}"
if [[ ! -f "$llvm/i686-w64-mingw32/lib/libkernel32.a" && -f "$root/tools/runic-compiler-recovery/mini-llvm/i686-w64-mingw32/lib/libkernel32.a" ]]; then
    llvm="$root/tools/runic-compiler-recovery/mini-llvm"
fi
# Source ZIP extraction may lose POSIX mode bits on wrapper scripts.
if [[ -f "$llvm/bin/lld-link" && -f "$llvm/bin/llvm-objdump" ]]; then
    chmod +x "$llvm/bin/lld-link" "$llvm/bin/llvm-objdump" 2>/dev/null || true
fi
clang="${CLANG_CL:-/usr/local/swift/usr/bin/clang-cl}"
out="${1:-/mnt/data/rw-v012-build}"
export TESV_EXE="${TESV_EXE:-$root/inputs/TESV.exe}"
python "$here/V015VisualContractTests.py"
python "$here/V016PresentationContractTests.py"
python "$here/V017FunctionalContracts.py"
python "$here/V019RegressionTests.py"
python "$here/V020RegressionTests.py"
python "$here/V017NativePreservationTests.py"
python "$here/ProfessionalArtTests.py"
python "$here/DisplayNameContractTests.py"
python "$here/qa/pro_art_qa.py"
python "$here/InventoryCountContractTests.py"
python "$here/ControlParityTests.py"
python "$here/Regression09Tests.py"
python "$here/UITaskContractTests.py"
python "$here/SpellLayoutTests.py"
python "$here/SpellNativeRouteTests.py"
clang++ -std=c++17 -Wall -Wextra -Werror -O2 "$here/PresentationTimelineTests.cpp" -o "${TMPDIR:-/tmp}/runic-presentation-tests"
"${TMPDIR:-/tmp}/runic-presentation-tests"
clang++ -std=c++17 -Wall -Wextra -Werror -O2 "$here/V018FunctionalTests.cpp" -o "${TMPDIR:-/tmp}/runic-v019-functional-tests"
"${TMPDIR:-/tmp}/runic-v019-functional-tests"
clang++ -std=c++17 -Wall -Wextra -Werror -O2 "$here/IconPolicyTests.cpp" -o "${TMPDIR:-/tmp}/runic-v019-icons-tests"
"${TMPDIR:-/tmp}/runic-v019-icons-tests"
clang++ -std=c++17 -Wall -Wextra -Werror -O2 "$here/V019StackTests.cpp" -o "${TMPDIR:-/tmp}/runic-v020-stack-tests"
"${TMPDIR:-/tmp}/runic-v020-stack-tests"
clang++ -std=c++17 -Wall -Wextra -Werror -O2 "$here/V020PoliciesTests.cpp" -o "${TMPDIR:-/tmp}/runic-v020-policy-tests"
"${TMPDIR:-/tmp}/runic-v020-policy-tests"
clang++ -std=c++17 -Wall -Wextra -Werror -O2 "$here/AdvancedEditorTests.cpp" -o "${TMPDIR:-/tmp}/runic-v021-editor-tests"
"${TMPDIR:-/tmp}/runic-v021-editor-tests"
clang++ -std=c++17 -Wall -Wextra -Werror -O2 "$here/AdvancedEditorStressTests.cpp" -o "${TMPDIR:-/tmp}/runic-v021-stress-tests"
"${TMPDIR:-/tmp}/runic-v021-stress-tests"
python "$here/V021IntegrationContracts.py"
python "$root/projects/RunicWheelLE/quality/validate.py" --profile dev --source "$here/RunicWheelV011.cpp" --ini "$here/RunicWheelLE.ini"
mkdir -p "$out"
export LLVM_MINGW_ROOT="$llvm"
"$clang" /nologo /c /O1 /EHa /GS- /GR- /W4 /clang:-target /clang:i686-pc-windows-msvc \
  "/I$root/sdk/skse-le-sdk" "/I$root/sdk/skse-le-sdk/skse" \
  "/Fo$out/RunicWheelLE.obj" "$here/RunicWheelV011.cpp"
"$llvm/bin/lld-link" /nologo /dll /noentry /machine:x86 /nodefaultlib \
  "/out:$out/RunicWheelLE.dll" "$out/RunicWheelLE.obj" \
  "$llvm/i686-w64-mingw32/lib/libmsvcrt.a" \
  "$llvm/i686-w64-mingw32/lib/libkernel32.a" \
  "$llvm/i686-w64-mingw32/lib/libuser32.a"
LLVM_MINGW_ROOT="$llvm" python "$here/verify-v011.py" "$out/RunicWheelLE.dll"
echo "PASS: $out/RunicWheelLE.dll (PRIVATE v0.21 ADVANCED EDITOR/STACK NAV, game-validation pending)"
