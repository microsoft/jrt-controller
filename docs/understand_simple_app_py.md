
- [1. Understanding the simple example application (first\_example\_py)](#1-understanding-the-simple-example-application-first_example_py)
  - [1.1. Implementation details](#11-implementation-details)
    - [1.1.1. Application state variables](#111-application-state-variables)
    - [1.1.2. Application configuration](#112-application-configuration)
    - [1.1.3. Callback handler](#113-callback-handler)
  - [1.2. Run the application](#12-run-the-application)
    - [1.2.1. Prerequisites](#121-prerequisites)
    - [1.2.2. Build the application](#122-build-the-application)
    - [1.2.3. Run the components](#123-run-the-components)
    - [1.2.4. Expected output](#124-expected-output)


# 1. Understanding the simple example application (first_example_py)

The logic of [first example py](../sample_apps/first_example_py/) is identical to the [first example c](../sample_apps/first_example_c/).

To see the details of this logic and expected behaviour, refer to [./understand_simple_app_c.md](./understand_simple_app_c.md)

For the python version, see [jrtc_app.py](../src/wrapper_apis/python/jrtc_app.py).  This file is a python wrapper used to interface to the API described in [first example c](../sample_apps/first_example_c/)..

The details of first_example_py is almost identical to first_example_c, in that the developer just has to define the state variables, the application configuration, amd the callback handler.  

This README will simply show how to write the app in python.  

## 1.1. Implementation details

### 1.1.1. Application state variables

```python
class AppStateVars(ctypes.Structure):
    _fields_ = [
        ("app", ctypes.POINTER(JrtcApp)),
        
        # add custom fields below
        ("agg_cnt", ctypes.c_int),
        ("received_counter", ctypes.c_int)
    ]    
```

### 1.1.2. Application configuration

```python
streams = [
    # GENERATOR_OUT_STREAM_IDX
    JrtcStreamCfg_t(
        JrtcStreamIdCfg_t(
            JRTC_ROUTER_REQ_DEST_ANY, 
            JRTC_ROUTER_REQ_DEVICE_ID_ANY, 
            b"FirstExample://jbpf_agent/data_generator_codeletset/codelet", 
            b"ringbuf"),
        True,   # is_rx
        None    # No AppChannelCfg 
    ),
    # SIMPLE_INPUT_IN_STREAM_IDX
    JrtcStreamCfg_t(
        JrtcStreamIdCfg_t(
            JRTC_ROUTER_REQ_DEST_NONE, 
            1, 
            b"FirstExample://jbpf_agent/simple_input_codeletset/codelet", 
            b"input_map"),
        False,  # is_rx
        None    # No AppChannelCfg 
    )
]
```

The application config is detailed below.

```python
app_cfg = JrtcAppCfg_t(
    b"FirstExample",                               # context
    100,                                           # q_size
    len(streams),                                  # num_streams
    (JrtcStreamCfg_t * len(streams))(*streams),    # streams
    10.0,                                          # initialization_timeout_secs
    0.1,                                           # sleep_timeout_secs
    1.0                                            # inactivity_timeout_secs
)
```

### 1.1.3. Callback handler

```python
def app_handler(timeout: bool, stream_idx: int, data_entry_ptr: ctypes.POINTER(struct_jrtc_router_data_entry), state_ptr: int):

    GENERATOR_OUT_STREAM_IDX = 0
    SIMPLE_INPUT_IN_STREAM_IDX = 1

    if timeout:
        # Timeout processing (not implemented in this example)
        pass

    else:

        # Dereference the pointer arguments
        state = ctypes.cast(state_ptr, ctypes.POINTER(AppStateVars)).contents        
        data_entry = data_entry_ptr.contents

        if stream_idx == GENERATOR_OUT_STREAM_IDX:

            state.received_counter += 1

            data_ptr = ctypes.cast(
                data_entry.data, ctypes.POINTER(ctypes.c_char)
            )
            raw_data = ctypes.string_at(
                data_ptr, ctypes.sizeof(ctypes.c_char) * 4
            )
            value = int.from_bytes(raw_data, byteorder="little")

            state.agg_cnt += value  # For now, just increment as an example

        if state.received_counter % 5 == 0 and state.received_counter > 0:

            aggregate_counter = simple_input()
            aggregate_counter.aggregate_counter = state.agg_cnt
            data_to_send = bytes(aggregate_counter)

            # get the inout stream
            sid = jrtc_app_get_stream(state.app, SIMPLE_INPUT_IN_STREAM_IDX)
            assert (sid is not None), "Failed to get SIMPLE_INPUT_IN_STREAM_IDX stream"

            # send the data
            res = jrtc_router_channel_send_input_msg(sid, data_to_send, len(data_to_send))
            assert res == 0, "Failed to send aggregate counter to input map"

            print(f"FirstExample: Aggregate counter so far is: {state.agg_cnt}")
```

## 1.2. Run the application

### 1.2.1. Prerequisites

Before running the sample apps, the *jrt-controller* must be built (see [README.md](../../README.md) for instructions).

### 1.2.2. Build the application

The application can be built using the following commands:
  ```sh
  cd $JRTC_PATH/sample_apps/first_example_py
  make
  ```

This will build the two sample codelets (`data_generator` and `simple_input`), the sample agent (`simple_agent_ipc`) and the *jrt-controller* application (`app1.so`).


### 1.2.3. Run the components

You will need to open five terminals.

* **Terminal 1:** 
  
  This is used to run the *jrt-controller*. 
  ```sh
  cd $JRTC_PATH/sample_apps/first_example_py
  ./run_jrtc.sh
  ```

* **Terminal 2:**

  This is used to run the test agent, where the example codelets will be loaded.
  ```sh
  cd $JRTC_PATH/sample_apps/first_example_py
  ./run_simple_agent_ipc.sh
  ```
  If successful, one of the log messages at the agent side should report `Registration succeeded`.

* **Terminal 3:**

  This is used to run the *jbpf* [reverse proxy](https://github.com/microsoft/jbpf/blob/main/docs/life_cycle_management.md) process, which enables the loading of codelets to the agent using a TCP socket (e.g., from a remote machine).

  ```sh
  cd $JRTC_PATH/sample_apps/first_example_py
  ./run_reverse_proxy.sh
  ```

* **Terminal 4:**

  This is used to run the *jrt-controller" decoder.
  
  ```sh
  cd $JRTC_PATH/sample_apps/first_example_py
  ./run_decoder.sh
  ```

* **Terminal 5:**

  This is used to load and unload the example application.
  ```sh
  cd $JRTC_PATH/sample_apps/first_example_py
  ./load_app.sh
  ```


### 1.2.4. Expected output

If the codelets and the app were loaded successfully, you should see the following output at the jrt-controller:
```
App1: Aggregate counter from codelet is 1
App2: Aggregate counter from codelet is 1
App1: Received aggregate counter 1 from output channel of App2
App1: Received aggregate counter 1 from input channel of App1
App1: Aggregate counter from codelet is 3
App2: Aggregate counter from codelet is 3
App1: Received aggregate counter 3 from output channel of App2
App1: Received aggregate counter 3 from input channel of App1
```

Similarly, you should see the following printed messages on the agent side from the simple input codelet:
```
[2025-03-06T11:54:08.746075Z] [JBPF_DEBUG]: Got aggregate value 1
[2025-03-06T11:54:09.746131Z] [JBPF_DEBUG]: Got aggregate value 3
```

The application can then be unloaded with the following command:
```sh
./unload_app.sh
```

