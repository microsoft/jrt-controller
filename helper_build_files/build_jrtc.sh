#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

source "/jrtc/helper_build_files/build_utils.sh"

## get the build flags and targets from env parameters
get_flags
echo -e "$OUTPUT"
echo FLAGS=$FLAGS

JRTC_OUT_DIR=/jrtc/out

rm -rf /jrtc/out
rm -rf /jrtc/build
mkdir /jrtc/build

cd /jrtc

## check clang-format
## please note that different versions of clang-format may have different output
if [[ "$CLANG_FORMAT_CHECK" == "1" ]]; then
    echo "Checking clang-format..."
    cat /jrtc/.clang-format
    DIRS=("src" "sample_apps" "tests" "tools" "wrapper_apis/c")
    cd /jrtc/
    echo The clang-format version is $(clang-format --version)
    for i in "${DIRS[@]}"; do
        echo "Checking clang-format in $i"
        find "$i" -iname "*.c" -o -iname "*.h" -o -iname "*.cpp" -o -iname "*.hpp" | xargs clang-format --dry-run --Werror
        if [ $? -ne 0 ]; then
            echo "Error: clang-format check failed."
            exit 1
        fi
    done
fi

cd /jrtc/build
cmake ../ $FLAGS

MAKE_FLAGS=""

if [[ "$VERBOSE" == "1" ]]; then
    echo "Building with VERBOSE=1"
    MAKE_FLAGS="VERBOSE=1"
fi

if ! make $MAKE_FLAGS; then
    echo "Error building!"
    exit 1
fi

## if "JRTC_COVERAGE" is set, run make jrtc_coverage
if [[ "$JRTC_COVERAGE" == "1" ]]; then
    make jrtc_coverage_xml
    cp jrtc_coverage.xml $JRTC_OUT_DIR
fi

## if "JRTC_COVERAGE_HTML" is set, run make jrtc_coverage_html
if [[ "$JRTC_COVERAGE_HTML" == "1" ]]; then
    make jrtc_coverage_html
    cp -R jrtc_coverage.html $JRTC_OUT_DIR
fi

## if "DOXYGEN" is set, run make doc
if [[ "$DOXYGEN" == "1" ]]; then
    make jrtc_doc
fi

if [[ "$RUN_TESTS" == "1" ]]; then
    export JRTC_PATH=/jrtc
    ## TODO: ODR (Linkage problem) isn't a big issue, but we should fix it at some point
    export ASAN_OPTIONS="detect_leaks=1:strict_memcmp=1:log_path=/tmp/asan.log:detect_odr_violation=0:verbosity=2:strict_init_order=true"
    export LSAN_OPTIONS="suppressions=/jrtc/lsan_agent.supp:verbosity=2"
    export RUST_BACKTRACE=full
    if ! ctest --output-on-failure --output-junit $JRTC_OUT_DIR/jrtc_tests.xml -F; then
        echo "Error running C tests!"
        exit 1
    fi
    ## if there are no tests, fail
    if [ ! -s $JRTC_OUT_DIR/jrtc_tests.xml ]; then
        echo "Error: No tests found!"
        exit 1
    fi
    cat /tmp/asan.log || true
    cd /jrtc/build
    if ! make jrtc-ctl-test; then
        echo "Error running jrtcctl tests!"
        exit 1
    fi
    if ! make jrtc-ctl-lint; then
        echo "Error running jrtc tests!"
        exit 1
    fi
fi

## move built files
mkdir -p /jrtc_out_lib
rm -rf /jrtc_out_lib/out
cp -r $JRTC_OUT_DIR /jrtc_out_lib
chmod -R 777 /jrtc_out_lib/out
