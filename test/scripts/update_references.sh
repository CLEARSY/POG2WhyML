#!/bin/bash

# Generate pog files from B machine files found in test/input/

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/../utils.sh"

testdir="${1:-$(dirname "$SCRIPT_DIR")}"

tests=$(find "$testdir" -type f -name '*.mch')

tests_total=0
ok_total=0
for test in $tests; do
    tests_total=$(($tests_total + 1))
    directory=$(dirname "$test")
    filename=$(basename "$test")
    component=$(basename -s .mch "$filename")
    echo "generating pog for $component"
    pushd "$directory" > /dev/null
    GENERATE_POG "$component"
    if [ $? -ne 0 ]; then
        echo "error generating pog for $component"
        popd > /dev/null
        continue
    fi
    ok_total=$(($ok_total + 1))
    popd > /dev/null
done

echo ""
echo "Summary: $ok_total/$tests_total POG files generated"
