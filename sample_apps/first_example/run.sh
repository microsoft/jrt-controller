#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
## example: sudo -E script -q -c "./run.sh" output.txt
## cat output.txt | grep "Aggregate counter so far is"

export PYTHONVERBOSE=0

# Check if JRTC_PATH is set
if [ -z "$JRTC_PATH" ]; then
    echo "Error: JRTC_PATH environment variable is not set. Please set it before running this script."
    exit 1
fi

# Array to track background process IDs
PIDS=()

# Function to run a command in the background and track its PID
run_background_command() {
    local cmd="$1"
    echo "Running in background: $cmd"
    eval "$cmd &"
    PIDS+=($!) # Store the PID of the background process
}

# Function to clean up all spawned processes
cleanup() {
    echo "Cleaning up spawned processes..."

    # Kill child processes of this script first
    pkill -P $$

    # Kill all tracked PIDs explicitly
    for pid in "${PIDS[@]}"; do
    if ps -p $pid > /dev/null 2>&1; then
        echo "Killing process with PID $pid"
        kill -9 $pid
    fi
    done

    # Additional cleanup: kill jrtc-related processes by name
    echo "Killing any remaining jrt-controller processes..."
    pgrep -f 'run_jrtc.sh' | xargs -r kill -9
    pgrep -f 'simple_agent_ipc' | xargs -r kill -9
    pgrep -f 'run_reverse_proxy.sh' | xargs -r kill -9
    pgrep -f 'run_decoder.sh' | xargs -r kill -9
    pgrep -f '/out/bin/jrtc' | xargs -r kill -9
    pgrep -f 'proxy --address /tmp/jbpf/jbpf_lcm_ipc' | xargs -r kill -9
    pgrep -f 'jrtc-ctl decoder run' | xargs -r kill -9  
    echo "Cleanup complete."
}

# Set trap to clean up processes on script exit
trap cleanup EXIT

# Trap Ctrl C
trap cleanup SIGINT

# Step 1: Run jrt-controller
run_background_command "cd $JRTC_PATH/sample_apps/first_example && source ../../setup_jrtc_env.sh && ./run_jrtc.sh"

sleep 3

# Step 2: Run Jbpf IPC agent
run_background_command "cd $JRTC_PATH/sample_apps/first_example && source ../../setup_jrtc_env.sh && ./run_simple_agent_ipc.sh"

sleep 1

# Step 3: Run reverse proxy
run_background_command "cd $JRTC_PATH/sample_apps/first_example && source ../../setup_jrtc_env.sh && ./run_reverse_proxy.sh"

sleep 1

# Step 4: Run decoder
run_background_command "cd $JRTC_PATH/sample_apps/first_example && source ../../setup_jrtc_env.sh && ./run_decoder.sh"

sleep 1

# Step 5: Load YAML (runs in the foreground)
echo "Running: Load YAML"
cd $JRTC_PATH/sample_apps/first_example && source ../../setup_jrtc_env.sh && ./load_app.sh

## Wait to see output
echo "Waiting for output..."
sleep 20

# Step 6: Unload YAML (runs in the foreground)
echo "Running: Unload YAML"
cd $JRTC_PATH/sample_apps/first_example && source ../../setup_jrtc_env.sh && ./unload_app.sh

echo "Test completed."
