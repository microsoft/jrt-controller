#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

if [ -z "$JRTC_PATH" ]; then
    echo "Error: JRTC_PATH environment variable is not set. Please set it before running this script."
    exit 1
fi

if ! pushd "$JRTC_PATH/helper_build_files"; then
    echo "Error: Failed to change directory to $JRTC_PATH/helper_build_files"
    exit 2
fi

TEST_CASES=(
    "test_apps/first_example_py"
    "sample_apps/advanced_example_py"
    "sample_apps/first_example_py"
    "sample_apps/first_example"
    "sample_apps/first_example_c"
    "sample_apps/advanced_example"
    "sample_apps/advanced_example_c"
)
echo "Running integration tests for the following cases: ${TEST_CASES[*]}"

JRTC_TESTS_OUTPUT=/tmp/jrtc_tests_output.log

for TEST in "${TEST_CASES[@]}"; do
    echo ".......................................................................................... Running test: $TEST .........................................................................................."
    rm -rf "$JRTC_TESTS_OUTPUT" || true
    touch "$JRTC_TESTS_OUTPUT" || { echo "Error: Failed to create $JRTC_TESTS_OUTPUT"; exit 2; }

    "$JRTC_PATH/helper_build_files/integration_tests.sh" "$TEST" |& tee "$JRTC_TESTS_OUTPUT"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
        echo "Error: Test script failed for $TEST"
        exit 3
    fi
  
    echo ".............................................Output.log: $TEST ............................................."
    timeout 10 tail -f "$JRTC_TESTS_OUTPUT" | grep -q .  # Wait until file is non-empty
    cat "$JRTC_TESTS_OUTPUT"

    if ! "$JRTC_PATH/$TEST/assert.sh" "$JRTC_TESTS_OUTPUT" "$TEST"; then
        echo "Test Assertion failed for $TEST"
        exit 4
    fi
        
    echo ".......................................................................................... Test Passed: $TEST .........................................................................................."
done

echo "All tests passed!"

popd || echo "Warning: popd failed, but continuing"
exit 0
