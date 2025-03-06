#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
set -ex

if [ -z "$JRTC_PATH" ]; then
    echo "Error: JRTC_PATH environment variable is not set. Please set it before running this script."
    exit 1
fi

pushd $JRTC_PATH/helper_build_files

TEST_CASES=("first_example" "first_example_c" "first_example_py" "advanced_example" "advanced_example_c")
JRTC_TESTS_OUTPUT=/tmp/jrtc_tests_output.log

for TEST in "${TEST_CASES[@]}"; do
    echo ".......................................................................................... Running test: $TEST .........................................................................................."
    rm -rf $JRTC_TESTS_OUTPUT || true

    $JRTC_PATH/helper_build_files/integration_tests.sh $TEST |& tee $JRTC_TESTS_OUTPUT
        
    echo ".............................................Output.log: $TEST ............................................."
    cat $JRTC_TESTS_OUTPUT

    if ! $JRTC_PATH/sample_apps/$TEST/assert.sh $JRTC_TESTS_OUTPUT; then
        echo "Test Assertion failed for $TEST"
        exit 1
    fi
        
    echo ".......................................................................................... Test Passed: $TEST .........................................................................................."
done

echo "All tests passed!"
