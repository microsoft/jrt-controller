Copyright (c) Microsoft Corporation.
Licensed under the MIT license.

This example demonstrates a simple jrt-controller application that loads two codelets to a simple jbpf agent.
The data_generator codelet increments a counter and sends the data to the jrt-controller app via an output map.
The application aggregates the counter, prints a message about the aggregate value and sends this
value to the simple_input codelet via a control_input channel. The simple_input codelet prints the received values.

This example assumes that you already have Jbpf and you have already built the Jbpf library without DPDK support (cmake option `-DUSE_DPDK=off`).
Also, it assumes that you are running all the below steps as root.

Steps to run the example.   The following examples use "ubuntu2204" as the OS, but "mariner" could be used too.

## Usage

Before running the sample apps build the project, see [README.md](../../README.md) for instructions to build bare metal or with containers.

## 1. Build the jbpf
  ```sh
  cd $JRTC_PATH/jbpf-protobuf/jbpf
  ./init_and_patch_submodules.sh
  source setup_jbpf_env.sh
  mkdir -p build
  cd build
  cmake ../
  make -j
  ```

## 2. Build the controller
  ```sh
  cd $JRTC_PATH
  ./init_submodules.sh
  source setup_jrtc_env.sh
  mkdir -p build
  cd build
  cmake ../
  make
  ```

## 3. Build all test entities
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example
  make
  ```

## 4. Open a terminal and run the controller ..
  ```sh
  cd $JRTC_PATH/sample_apps/first_example
  source ../../setup_jrtc_env.sh
  sudo -E ./run_jrtc.sh
  ```

## 5. Open a separate terminal and run the Jbpf IPC agent
  ```sh
  cd $JRTC_PATH/sample_apps/first_example
  source ../../setup_jrtc_env.sh
  sudo -E ./run_simple_agent_ipc.sh
  ```

If successful, one of the log messages at the agent side should report `Registration succeeded:`.

## 6. Open a separate terminal and run reverse proxy
  ```sh
  cd $JRTC_PATH/sample_apps/first_example
  source ../../setup_jrtc_env.sh
  sudo -E ./run_reverse_proxy.sh
  ```

## 6. Open a separate terminal and run decoder
  ```sh
  cd $JRTC_PATH/sample_apps/first_example
  source ../../setup_jrtc_env.sh
  sudo -E ./run_decoder.sh
  ```  

## 7. Open a separate terminal and load the yaml
  ```sh
  cd $JRTC_PATH/sample_apps/first_example
  source ../../setup_jrtc_env.sh
  sudo -E ./load_app.sh
  ```

Unload the yaml:
```sh
# Terminal 5
source ../../setup_jrtc_env.sh
./unload_app.sh
```

### Running with containers

To build the example from scratch, we run the following commands:
```sh
docker run --rm -it \
  -v $(dirname $(dirname $(pwd))):/jrtc \
  -w /jrtc/sample_apps/first_example \
    jrtc-$OS:latest make
```

Start jrtc:
```sh
# Terminal 1
docker run --rm -it --net=host \
  -v /tmp/jbpf:/tmp/jbpf \
  -v /dev/shm:/dev/shm \
  -v $(dirname $(dirname $(pwd))):/jrtc \
  -w /jrtc/sample_apps/first_example \
    jrtc-$OS:latest ./run_jrtc.sh
```

In another terminal, run the Jbpf IPC agent:
```sh
# Terminal 2
docker run --rm -it \
  -v /tmp/jbpf:/tmp/jbpf \
  -v /dev/shm:/dev/shm \
  -v $(dirname $(dirname $(pwd))):/jrtc \
  -w /jrtc/sample_apps/first_example \
    jrtc-$OS:latest ./run_simple_agent_ipc.sh
```

Start the reverse proxy:
```sh
# Terminal 3
docker run --rm -it --net=host \
  -v /tmp/jbpf:/tmp/jbpf \
  -v /dev/shm:/dev/shm \
  -v $(dirname $(dirname $(pwd))):/jrtc \
  -w /jrtc/sample_apps/first_example \
    jrtc-$OS:latest ./run_reverse_proxy.sh
```

Run the local decoder:
```sh
# Terminal 4
docker run --rm -it --net=host \
  -v $(dirname $(dirname $(pwd))):/jrtc \
  -w /jrtc/sample_apps/first_example \
    jrtc-$OS:latest ./run_decoder.sh
```

Load the yaml:
```sh
# Terminal 5
docker run --rm -it --net=host \
  -v $(dirname $(dirname $(pwd))):/jrtc \
  -w /jrtc/sample_apps/first_example \
    jrtc-$OS:latest ./load_app.sh
```

If the codelets and the app were loaded successfully, you should see the following output at the jrt-controller:
```
App 1: Aggregate counter so far is 15
App 1: Aggregate counter so far is 55
...
```

Similarly, you should see the following printed messages on the agent side:
```
[JBPF_DEBUG]: Got aggregate value 15
[JBPF_DEBUG]: Got aggregate value 55
...
```

Unload the yaml:
```sh
# Terminal 5
docker run --rm -it --net=host \
  -v $(dirname $(dirname $(pwd))):/jrtc \
  -w /jrtc/sample_apps/first_example \
    jrtc-$OS:latest ./unload_app.sh
```
