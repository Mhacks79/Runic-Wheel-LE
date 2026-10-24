#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
python Regression09Tests.py
python ControlParityTests.py
cxx="${CXX:-clang++}"
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror TimePolicyTests.cpp -o /tmp/rw-v09-policy-test
/tmp/rw-v09-policy-test
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror GestureParityTests.cpp -o /tmp/rw-v09-gesture-test
/tmp/rw-v09-gesture-test
"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer GestureParityTests.cpp -o /tmp/rw-v09-gesture-sanitized
/tmp/rw-v09-gesture-sanitized
"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer TimePolicyTests.cpp -o /tmp/rw-v09-policy-sanitized
/tmp/rw-v09-policy-sanitized
