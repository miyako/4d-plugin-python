#!/bin/bash
# Builds the synthetic-boundary ownership harness against a source tree with
# ASan/UBSan and runs every test in its own process.
#
#   tests/ownership/run.sh <source-root> <fixed|baseline>
#
# <source-root> provides the plugin sources, the bundled 4D Plugin API and
# the bundled jsoncpp. The harness itself always comes from this script's
# directory, so the SAME harness can be run against an immutable baseline
# (git archive of the base SHA) and against the fixed tree. The embedded
# Python interpreter is stubbed in the harness; libpython is never linked.
#
# In "fixed" mode every test must pass. In "baseline" mode the tests named
# in EXPECTED_BASELINE_FAILURES must fail and every other test (the
# controls) must pass; anything else is a harness or model error.
set -euo pipefail

SRC=$(cd "$1" && pwd)
MODE=$2
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT

EXPECTED_BASELINE_FAILURES="
defect_cmd_python_variables_stringify_leak
defect_cmd_python_list_string_element_leak
defect_cmd_python_list_dict_element_leak
defect_cmd_python_list_list_element_leak
defect_fourd_call_text_result_leak
defect_fourd_call_object_result_leak
defect_fourd_call_collection_result_leak
defect_cmd_python_fourd_call_end_to_end
"

SAN="-g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=undefined"
# Same header search paths as the Xcode project.
INC=(-I "$SRC/4D Plugin API" -I "$SRC/support" -I "$SRC" -I "$SRC/include")
WARN="-w"

objs=()
cc() { # <lang> <file> [extra flags...]
    local lang=$1 file=$2; shift 2
    local o="$OUT/$(basename "$file").o"
    xcrun clang $SAN $WARN -x "$lang" "${INC[@]}" "$@" -c "$file" -o "$o"
    objs+=("$o")
}

cc c "$SRC/4D Plugin API/4DPluginAPI.c" -Wno-multichar -Wno-error=int-conversion
cc objective-c++ "$SRC/support/4DPlugin-JSON.cpp" -std=c++17 -fno-objc-arc
# engine_harness.cpp #includes 4DPlugin-python.cpp from $SRC.
cc objective-c++ "$HERE/engine_harness.cpp" -std=c++17 -fno-objc-arc

xcrun clang++ $SAN "${objs[@]}" "$SRC/a/libjsoncpp.a" -framework Cocoa -framework CoreFoundation -o "$OUT/harness"

export ASAN_OPTIONS=abort_on_error=0:halt_on_error=1:detect_container_overflow=0
export UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1

bad=0
for t in $("$OUT/harness" --list); do
    if "$OUT/harness" "$t" >"$OUT/$t.log" 2>&1; then got=pass; else got=fail; fi
    expect=pass
    if [ "$MODE" = baseline ] && grep -qx "$t" <<<"$EXPECTED_BASELINE_FAILURES"; then expect=fail; fi
    if [ "$got" = "$expect" ]; then status=OK; else status=UNEXPECTED; bad=1; fi
    printf '%-12s %-5s (expected %-4s) %s\n' "$status" "$got" "$expect" "$t"
    if [ "$got" = fail ]; then sed 's/^/    | /' "$OUT/$t.log"; fi
    # A sanitizer report or harness error is never an expected outcome.
    if grep -qE 'ERROR: AddressSanitizer|runtime error:|HARNESS ERROR' "$OUT/$t.log"; then
        echo "UNEXPECTED   sanitizer/harness error in $t"; bad=1
    fi
done

if [ $bad -ne 0 ]; then echo "RESULT ($MODE): FAILED"; exit 1; fi
echo "RESULT ($MODE): all outcomes as expected"
