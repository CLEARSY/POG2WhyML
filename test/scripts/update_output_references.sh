#!/bin/bash

# Regenerate output references for all tests.
# Usage: update_output_references.sh <pog2why_binary> [test_dir]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTDIR="${2:-$(dirname "$SCRIPT_DIR")}"
POG2WHYML="${1:-$(find "$TESTDIR/../build" -name pog2why -type f | head -1)}"

if [ -z "$POG2WHYML" ] || ! [ -x "$POG2WHYML" ]; then
    echo "Error: pog2why binary not found or not executable: $POG2WHYML" >&2
    exit 1
fi

ok=0
fail=0
declare -a failing_tests
declare -a failing_stderr

for inputdir in "$TESTDIR"/input/test*/; do
    testid=$(basename "$inputdir")
    pogfile=$(find "$inputdir" -name "*.pog" | head -1)
    [ -z "$pogfile" ] && continue
    comp=$(basename "$pogfile" .pog)
    refdir="$TESTDIR/output/$testid/reference"
    mkdir -p "$refdir"
    "$POG2WHYML" -A -i "$pogfile" -o "$refdir/$comp.why" \
        > "$refdir/stdout" 2> "$refdir/stderr"
    exitcode=$?
    echo $exitcode > "$refdir/exitcode"
    if [ $exitcode -ne 0 ]; then
        echo "FAIL: $testid"
        failing_tests+=("$testid")
        failing_stderr+=("$(head -1 "$refdir/stderr")")
        ((fail++))
    else
        echo " OK:  $testid"
        ((ok++))
    fi
done

echo ""
echo "Summary: $ok OK, $fail failed"

# Write TODO.md at repo root
REPOROOT="$(dirname "$TESTDIR")"
TODOFILE="$REPOROOT/TODO.md"

if [ ${#failing_tests[@]} -eq 0 ]; then
    rm -f "$TODOFILE"
    exit 0
fi

{
    echo "# TODO"
    echo ""
    echo "Tests failing to produce valid pog2why output:"
    echo ""
    for i in "${!failing_tests[@]}"; do
        testid="${failing_tests[$i]}"
        pogfile=$(find "$TESTDIR/input/$testid" -name "*.pog" | head -1)
        comp=$(basename "$pogfile" .pog)
        stderr_msg=$(cat "$TESTDIR/output/$testid/reference/stderr")
        echo "- $testid ($comp)"
        if [ -n "$stderr_msg" ]; then
            echo "  \`\`\`"
            echo "$stderr_msg" | head -5 | sed 's/^/  /'
            echo "  \`\`\`"
        fi
    done
} > "$TODOFILE"

echo ""
echo "Failing tests written to $TODOFILE"
