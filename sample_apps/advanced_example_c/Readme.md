Copyright (c) Microsoft Corporation.
Licensed under the MIT license.

- [1. Intructions](#1-intructions)
- [2. Steps](#2-steps)
  - [2.1. Build the jbpf](#21-build-the-jbpf)
  - [2.2. Build the jrtc](#22-build-the-jrtc)
  - [2.3. Build all test entities](#23-build-all-test-entities)
  - [2.5. Open a terminal and run jrtc ..](#25-open-a-terminal-and-run-jrtc-)
  - [2.6. Open a separate terminal and run the Jbpf IPC agent](#26-open-a-separate-terminal-and-run-the-jbpf-ipc-agent)
  - [2.7. Open a separate terminal and run reverse proxy](#27-open-a-separate-terminal-and-run-reverse-proxy)
  - [2.7. Open a separate terminal and run reverse proxy](#27-open-a-separate-terminal-and-run-reverse-proxy-1)
  - [2.8. Open a separate terminal and load the yaml](#28-open-a-separate-terminal-and-load-the-yaml)
  - [2.9. To unload the codelets and the application, use the following command:](#29-to-unload-the-codelets-and-the-application-use-the-following-command)

# 1. Intructions

This example demonstrates a simple JRTC application that loads two codelets to a simple jbpf agent.
The data_generator codelet increments a counter and sends the data to the JRTC app via an output map.
The JRTC application aggregates the counter, prints a message about the aggregate value and sends this
value to the simple_input codelet via a control_input channel. The simple_input codelet prints the received values.

This example assumes that you already have Jbpf and you have already built the Jbpf library without DPDK support (cmake option `-DUSE_DPDK=off`).
Also, it assumes that you are running all the below steps as root.

Steps to run the example.   The followinf examples use "ubuntu2204" as the OS, but "mariner" could be used too.

# 2. Steps

## 2.1. Build the jbpf
  ```sh
  cd $JRTC_PATH/jbpf-protobuf/jbpf
  ./init_and_patch_submodules.sh
  source setup_jbpf_env.sh
  mkdir -p build
  cd build
  cmake ../
  make -j
  ```

## 2.2. Build the jrtc
  ```sh
  cd $JRTC_PATH
  ./init_submodules.sh
  source setup_jrtc_env.sh
  mkdir -p build
  cd build
  cmake ../
  make
  ```

## 2.3. Build all test entities
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  make
  ```

## 2.5. Open a terminal and run jrtc ..
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  source ../../setup_jrtc_env.sh
  sudo -E ./run_jrtc.sh
  ```

## 2.6. Open a separate terminal and run the Jbpf IPC agent
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  source ../../setup_jrtc_env.sh
  sudo -E ./run_simple_agent_ipc.sh
  ```

If successful, one of the log messages at the agent side should report `Registration succeeded:`.

## 2.7. Open a separate terminal and run reverse proxy
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  source ../../setup_jrtc_env.sh
  sudo -E ./run_reverse_proxy.sh
  ```

## 2.7. Open a separate terminal and run reverse proxy
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  source ../../setup_jrtc_env.sh
  sudo -E ./run_decoder.sh
  ```

## 2.8. Open a separate terminal and load the yaml
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  source ../../setup_jrtc_env.sh
  sudo -E ./load_app.sh
  ```

## 2.9. To unload the codelets and the application, use the following command:
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  source ../../setup_jrtc_env.sh
  sudo -E ./unload_app.sh
  ```
