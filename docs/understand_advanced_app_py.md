
- [1. Understanding the advanced example application (advanced\_example\_c)](#1-understanding-the-advanced-example-application-advanced_example_py)
  - [1.1. Application description](#11-application-description)
    - [1.1.1. Automatic serialization and routing](#111-automatic-serialization-and-routing)
    - [1.1.2. Messages between applications](#112-messages-between-applications)
  - [1.2. Implementation details - AdvancedExample1](#12-implementation-details---advancedexample1)
    - [1.2.1. Application state variables](#121-application-state-variables)
    - [1.2.2. Application configuration](#122-application-configuration)
    - [1.2.3. Callback handler](#123-callback-handler)
  - [1.3. Implementation details - AdvancedExample2](#13-implementation-details---advancedexample2)
    - [1.3.1. Application state variables](#131-application-state-variables)
    - [1.3.2. Application configuration](#132-application-configuration)
    - [1.3.3. Callback handler](#133-callback-handler)
  - [1.4. Run the application](#14-run-the-application)
    - [1.4.1. Prerequisites](#141-prerequisites)
    - [1.4.2. Build the application](#142-build-the-application)
    - [1.4.3. Run the components](#143-run-the-components)
    - [1.4.4. Expected output](#144-expected-output)


# 1. Understanding the advanced example application (advanced_example_py)

This [advanced example](../sample_apps/advanced_example_py/) builds on the basic concepts introduced in the [simple example](./understand_simple_app_c.md) and demonstrates more advanced capabilities offered by the *jrt-controller*.


## 1.1. Application description

This example involves two deployments.

The [first](../sample_apps/advanced_example_py/advanced_example1.yaml), called `AdvancedExample1`, consists of one *jrt-controller* application `app1` and two codelets:

* The [data_generator codelet](../sample_apps/jbpf_codelets/data_generator/data_generator_codelet.c) increments a counter every time it is called and sends it to `app1`. 
  It is the same as the one in the simple example. 

* The [simple_input protobuf codelet](../sample_apps/jbpf_codelets/simple_input_protobuf/simple_input_program.c) receives an input from the *jrt-controller* application and prints it on the console. 
  It is the same as the one in the simple example, except that it uses a [protobuf schema](../sample_apps/jbpf_codelets/simple_input_protobuf/simple_input.proto) for exposing the data to the *jrt-controller*.
This enables the serialized data forwarding to a *Data collection and control* point, in addition to exposing the data to  `app2`.

The [second](../sample_apps/advanced_example_py/advanced_example2.yaml) deployment, called `AdvancedExample2`, consists of a *jrt-controller* application `app2` and a single codelet:

* The [data_generator_protobuf codelet](../sample_apps/jbpf_codelets/data_generator_protobuf/data_generator_pb_codelet.c) is the same as the *data_generator codelet* of `app1`, except that it uses a [protobuf schema](../sample_apps/jbpf_codelets/data_generator_protobuf/generated_data.proto) for exposing the data to the *jrt-controller*.

The deployments have the following application logic:
* [app1](../sample_apps/advanced_example_py/advanced_example1.c) reads data from the *data_generator* codelet, updates a counter, and sends the result back to the *simple_input* codelet. This part is the same as in the [simple_example app](../sample_apps/first_example_c/first_example.c). 
  `app1` also receives data from `app2` over two streams (one input and one output) and prints it on screen. 

* [app2](../sample_apps/advanced_example_py/advanced_example2.c) reads data from the `data_generator_protobuf` codelet, updates a counter, and sends the updated counters to `app2` over both the input and output streams. 

There are several new elements illustrated in the advanced example that we discuss in more details. 

### 1.1.1. Automatic serialization and routing

One new element is the [data_generator_protobuf codelet](../sample_apps/jbpf_codelets/data_generator_protobuf/data_generator_pb_codelet.c) of deployment `AdvancedExample2`, which introduces the concept of automatic data serialization. 
Instead of manually defining a C struct for the data sent to `app2` through the output stream `ringbuf`, the codelet leverages a protobuf schema definition, which allows the same data to also be sent to one or more *Data collection and control* end-points over the network in a serialized format (protobuf). 
The protobuf schema that is employed for this example can be found [here](../sample_apps/jbpf_codelets/simple_input_protobuf/simple_input.proto).


When [building](../sample_apps/jbpf_codelets/data_generator_protobuf/Makefile) the binaries of the deployment `AdvancedExample2` using `make`, a file called `generated_data.pb.h` is auto-generated, containing a C struct called `example_msg_pb`.
The build process also generates a protobuf descriptor called (`generated_data.pb`) and a serde (serialization/deserialization) library called (`generated_data:example_msg_pb_serializer.so`).


The codelet can use the `example_msg_pb` structure to populate the output data, just as any other user-defined C struct:
```C
out = (_example_msg_pb*)c;
out->cnt = cnt;
jbpf_ringbuf_output(&ringbuf, out, sizeof(example_msg_pb));
```
When the message is sent to the output channel by the jbpf codelet using the `jbpf_ringbuf_output()` API call, it is received by the *jrt-controller* applications (`app2` in this case) without any serialization overhead (plain memory and with zero-copy), just as in the simple example.

However, in the case of this advanced example, the deployment also forwards the message to a remote UDP socket end-point by specifying a [forwarding policy](../sample_apps/jbpf_codelets/data_generator_protobuf/codeletset.yaml) in the codelet descriptor file:

```
out_io_channel:
    - name: ringbuf
    forward_destination: DestinationUDP
    serde:
        file_path: ${JRTC_PATH}/sample_apps/jbpf_codelets/data_generator_protobuf/generated_data:example_msg_pb_serializer.so
        protobuf:
        package_path: ${JRTC_PATH}/sample_apps/jbpf_codelets/data_generator_protobuf/generated_data.pb
        msg_name: example_msg_pb
```
Here, the descriptor file specifies that any data sent through the output stream with name `ringbuf` should be routed to the *Data collection and control* end-point called `DestinationUDP` (the UDP socket).
When forwarded, the data should be serialized using the protobuf descriptor and serde library specified under the `serde` and `protobuf` sections (generated through the build process explained previously).

The *jrt-controller* is bundled with a [forwarding application](../src/controller/jrtc_north_io_app.c), which is always enabled and subscribed to all streams with forwardng destination `JRTC_ROUTER_DEST_UDP` (the `DestinationUDP` end-point).
The forwarding application fetches data for any relevant stream, serializes it using the provided serialization information for the stream and sends it to the destination UDP socket over the network.
The destination IP and UDP port used by the forwarding application are defined through the env variables `AA_NET_OUT_FQDN` and `AA_NET_OUT_PORT` correspondingly.

Users can implement their own UDP end-point to handle the output data sent by the forwarding application (e.g., as part of their SMO), but *jrt-controller* also bundles a basic end-point with the [jrtc-ctl](jrtctl.md) tool, which can be useful for testing purposes.
The end-point, called `jrtc-ctl decoder`, simply receives the data via a UDP port, deserializes them, and prints them on screen in JSON format.

### 1.1.2. Messages between applications

In the previous example, a *jbpf* codelet was the creator of each channel. 
*jrt-controller* applications can also communicate amongst themselves using channels.
As with codelets, there are two types of application channels, *input* and *output* (see [here](../docs/streams.md) for more details).

For example, *app2* creates an instance of an output channel using the following definition:
```python
{
    {
        JRTC_ROUTER_REQ_DEST_NONE, 
        jrtc_device_id, 
        "AdvancedExample1://intraapp", 
        "buffer_input"
    },
    true,  // is_rx
    &(JrtcAppChannelCfg_t){false, 100, sizeof(simple_input_pb)}
}
```
As with the case of codelets, `app1` subscribes to this stream to receive data produced by `app2`.

The example also demonstrates how the same can be done for input channels, with `app1` creating the channel and `app2` sending data to it.

Note that the channel creation can optionally support de/serialization function if the data needs to be exported to external end-points. 

## 1.2. Implementation details - AdvancedExample1

Here are the details given for the advanced_example. 

### 1.2.1. Application state variables

In the case of the advanced_example1, it has additional field __"agg_cnt"__.

```python
class AppStateVars(ctypes.Structure):
    _fields_ = [
        ("app", ctypes.POINTER(JrtcApp)),
        
        # add custom fields below
        ("agg_cnt", ctypes.c_int32)
    ]
```

### 1.2.2. Application configuration

The streams of the applications are defined as below. It can be seen that these are as per described in [./understand_advanced_app.md](./understand_advanced_app.md).

```python
    streams = [
        # GENERATOR_OUT_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_ANY, 
                JRTC_ROUTER_REQ_DEVICE_ID_ANY, 
                b"AdvancedExample1://jbpf_agent/data_generator_codeletset/codelet", 
                b"ringbuf"),
            True,   # is_rx
            None    # No AppChannelCfg 
        ),
        # APP2_OUT_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_ANY, 0, 
                b"AdvancedExample2://intraapp", b"buffer"),
            True,   # is_rx
            None    # No AppChannelCfg 
        ),
        # SIMPLE_INPUT_CODELET_IN_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_NONE, 1, 
                b"AdvancedExample1://jbpf_agent/simple_input_pb_codeletset/codelet", b"input_map"),
            False,  # is_rx
            None    # No AppChannelCfg 
        ),
        # APP1_IN_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_NONE, 0, 
                b"AdvancedExample1://intraapp", b"buffer_input"),
            True,   # is_rx
            ctypes.POINTER(JrtcAppChannelCfg_t)(JrtcAppChannelCfg_t(False, 100, ctypes.sizeof(simple_input_pb)))
        )
    ]
```

In this example, the 4th stream also specifies that an "application" channel must be created.  Here are fields which are set ..
```sh
is_output: false  // Indicates if the channel is for output
num_elems: 100   // Number of elements in the channel
elem_size: sizeof(simple_input_pb)   // Size of each element in bytes
```

This is specifically creating an "input" channel for the AdvancedExample1 app.

The application config is detailed below.

```python
app_cfg = JrtcAppCfg_t(
    b"AdvancedExample1",                   # context
    100,                                  # q_size
    len(streams),                         # num_streams
    (JrtcStreamCfg_t * len(streams))(*streams),    # streams
    10.0,                                 # initialization_timeout_secs
    0.1,                                  # sleep_timeout_secs
    1.0                                   # inactivity_timeout_secs
)
```

### 1.2.3. Callback handler

```python
def app_handler(timeout: bool, stream_idx: int, data_entry_ptr: ctypes.POINTER(struct_jrtc_router_data_entry), state_ptr: int):

    GENERATOR_OUT_STREAM_IDX = 0
    APP2_OUT_STREAM_IDX = 1
    SIMPLE_INPUT_CODELET_IN_STREAM_IDX = 2
    APP1_IN_STREAM_IDX = 3

    if timeout:
        # Timeout processing (not implemented in this example)
        pass

    else:

        # Dereference the pointer arguments
        state = ctypes.cast(state_ptr, ctypes.POINTER(AppStateVars)).contents        
        data_entry = data_entry_ptr.contents

        if stream_idx == GENERATOR_OUT_STREAM_IDX:
            # Extract data from the received entry
            data = ctypes.cast(data_entry.data, ctypes.POINTER(example_msg)).contents
            state.agg_cnt += data.cnt
            print(f"App1: Aggregate counter from codelet is {state.agg_cnt}", flush=True)

            data_to_send = bytes(state.agg_cnt)
            data_len = 4

            # Send the aggregate counter back to the input codelet
            aggregate_counter = simple_input_pb()
            res = jrtc_app_router_channel_send_input_msg(state.app, SIMPLE_INPUT_CODELET_IN_STREAM_IDX, data_to_send, data_len)
            assert res == 0, "Failure in jrtc_router_channel_send_input_msg"

        elif stream_idx == APP2_OUT_STREAM_IDX:
            # Data received from App2 output channel
            appdata = ctypes.cast(data_entry.data, ctypes.POINTER(simple_input)).contents
            print(f"App1: Received aggregate counter {appdata.aggregate_counter} from output channel of App2", flush=True)

        elif stream_idx == APP1_IN_STREAM_IDX:
            # Data received from App1 input channel (by App2)
            appdata = ctypes.cast(data_entry.data, ctypes.POINTER(simple_input)).contents
            print(f"App1: Received aggregate counter {appdata.aggregate_counter} from input channel of App1", flush=True)

        else:
            print(f"App1: Got some unexpected message (stream_index={stream_idx})", flush=True)
            assert False
```

## 1.3. Implementation details - AdvancedExample2

Here are the details given for the advanced_example. 

### 1.3.1. Application state variables

In the case of the advanced_example1, it has additional fields __"agg_cnt"__ and __"received_counter"__.

```python
class AppStateVars(ctypes.Structure):
    _fields_ = [
        ("app", ctypes.POINTER(JrtcApp)),
        
        # add custom fields below
        ("agg_cnt", ctypes.c_int32),
        ("received_counter", ctypes.c_int)
    ]  
```

### 1.3.2. Application configuration

The streams of the applications are defined as below. It can be seen that these are as per described in [./understand_advanced_app.md](./understand_advanced_app.md).

```python
streams = [
    # GENERATOR_PB_OUT_STREAM_IDX = 0
    JrtcStreamCfg_t(
        JrtcStreamIdCfg_t(
            JRTC_ROUTER_REQ_DEST_ANY, 
            JRTC_ROUTER_REQ_DEVICE_ID_ANY, 
            b"AdvancedExample2://jbpf_agent/data_generator_pb_codeletset/codelet", 
            b"ringbuf"),
        True,   # is_rx
        None    # No AppChannelCfg 
    ),
    # APP2_OUT_STREAM_IDX
    JrtcStreamCfg_t(
        JrtcStreamIdCfg_t(
            JRTC_ROUTER_REQ_DEST_NONE, 
            0, 
            b"AdvancedExample2://intraapp", 
            b"buffer"),
        False,  # is_rx
        None    # No AppChannelCfg 
    ),
    # APP1_IN_STREAM_IDX
    JrtcStreamCfg_t(
        JrtcStreamIdCfg_t(
            JRTC_ROUTER_REQ_DEST_NONE, 
            0, 
            b"AdvancedExample1://intraapp", 
            b"buffer_input"),
        False,  # is_rx
        None    # No AppChannelCfg 
    )
]
```

In this example, the 4nd stream also specifies that an "application" channel must be created.  Here are fields which are set ..
```sh
is_output: true  // Indicates if the channel is for output
num_elems: 100   // Number of elements in the channel
elem_size: sizeof(simple_input_pb)   // Size of each element in bytes
```
This is specifically creating an "output" channel for the AdvancedExample2 app.

The application config is detailed below.

```python
app_cfg = JrtcAppCfg_t(
    b"AdvancedExample2",                   # context
    100,                                  # q_size
    len(streams),                         # num_streams
    (JrtcStreamCfg_t * len(streams))(*streams), # streams
    10.0,                                 # initialization_timeout_secs
    0.1,                                  # sleep_timeout_secs
    1.0                                   # inactivity_timeout_secs
)
```

### 1.3.3. Callback handler

```C
// Function for handling received data (as in Python)
def app_handler(timeout: bool, stream_idx: int, data_entry_ptr: ctypes.POINTER(struct_jrtc_router_data_entry), state_ptr: int):

    GENERATOR_PB_OUT_STREAM_IDX = 0
    APP2_OUT_STREAM_IDX = 1
    APP1_IN_STREAM_IDX = 2

    if timeout:
        # Timeout processing (not implemented in this example)
        pass

    else:
        # Dereference the pointer arguments
        state = ctypes.cast(state_ptr, ctypes.POINTER(AppStateVars)).contents        
        data_entry = data_entry_ptr.contents

        if stream_idx == GENERATOR_PB_OUT_STREAM_IDX:

            state.received_counter += 1

            data_ptr = ctypes.cast(
                data_entry.data, ctypes.POINTER(ctypes.c_char)
            )
            raw_data = ctypes.string_at(
                data_ptr, ctypes.sizeof(ctypes.c_char) * 4
            )
            value = int.from_bytes(raw_data, byteorder="little")

            state.agg_cnt += value  # For now, just increment as an example

            print(f"App2: Aggregate counter from codelet is {state.agg_cnt}", flush=True)

            # Get a buffer to write the output data
            counter = simple_input_pb()
            counter.aggregate_counter = state.agg_cnt

            # Send the data to the App2 output channel
            res = jrtc_app_router_channel_send_output(state.app, APP2_OUT_STREAM_IDX, counter.SerializeToString())
            assert res == 0, "Error returned from jrtc_router_channel_send_output"

            # Send the data to the App1 input channel
            aggregate_counter = simple_input_pb()
            aggregate_counter.aggregate_counter = state.agg_cnt

            res = jrtc_app_router_channel_send_input_msg(
                state.app, APP1_IN_STREAM_IDX, aggregate_counter.SerializeToString(), len(aggregate_counter.SerializeToString())
            )
            assert res == 0, "Failure in jrtc_router_channel_send_input_msg"

        else:
            print(f"App1: Got some unexpected message (stream_index={stream_idx})", flush=True)

        if state.received_counter % 5 == 0 and state.received_counter > 0:
            print(f"App2: Aggregate counter so far is {state.agg_cnt}", flush=True)
```

In the above handler it can be seen the app will send to its own "output" channel, which is APP2_OUT_SIDX.  To do this it calls "jrtc_app_router_channel_send_output".

## 1.4. Run the application

### 1.4.1. Prerequisites

Before running the sample apps, the *jrt-controller* must be built (see [README.md](../README.md) for instructions).

### 1.4.2. Build the application

The application can be built using the following commands:
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_py
  make
  ```

This will build the all the sample codelets (`data_generator`, `data_generator_protobuf`, `simple_input` and `simple_input_protobuf`), the sample agent (`simple_agent_ipc`) and the *jrt-controller* application (`app1.so`).


### 1.4.3. Run the components

You will need to open five terminals.

* **Terminal 1:** 
  
  This is used to run the *jrt-controller*. 
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_py
  ./run_jrtc.sh
  ```

* **Terminal 2:**

  This is used to run the test agent, where the example codelets will be loaded.
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_py
  ./run_simple_agent_ipc.sh
  ```
  If successful, one of the log messages at the agent side should report `Registration succeeded`.

* **Terminal 3:**

  This is used to run the *jbpf* [reverse proxy](https://github.com/microsoft/jbpf/blob/main/docs/life_cycle_management.md) process, which enables the loading of codelets to the agent using a TCP socket (e.g., from a remote machine).

  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_py
  ./run_reverse_proxy.sh
  ```

* **Terminal 4:**

  This is used to run the *jrt-controller" decoder.
  
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_py
  ./run_decoder.sh
  ```

* **Terminal 5:**

  This is used to load and unload the example application.
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_py
  ./load_app.sh
  ```


### 1.4.4. Expected output

If the codelets and the app were loaded successfully, you should see the following output at the jrt-controller:
```
App1: Aggregate counter from codelet is 1
App2: Aggregate counter from codelet is 1
App1: Received aggregate counter 1 from output channel of App2
App1: Received aggregate counter 1 from input channel of App1
App2: Aggregate counter from codelet is 3
App1: Aggregate counter from codelet is 3
App1: Received aggregate counter 3 from output channel of App2
App1: Received aggregate counter 3 from input channel of App1
...
```

Similarly, you should see the following printed messages on the agent side from the simple input codelet:
```
[2025-02-21T17:57:47.414608Z] [JBPF_DEBUG]: Got aggregate value 1
[2025-02-21T17:57:48.414826Z] [JBPF_DEBUG]: Got aggregate value 3
...
```

In addition, you should also see messages created by the `data_generator_protobuf` codelet being printed by the decoder:
```json
recv "00101ae42858fb419fb49d5613ddc6cf0801"(18)
INFO[0008] REC: {"cnt":1} streamUUID=00101ae4-2858-fb41-9fb4-9d5613ddc6cf
recv "00101ae42858fb419fb49d5613ddc6cf0802"(18)
INFO[0009] REC: {"cnt":2} streamUUID=00101ae4-2858-fb41-9fb4-9d5613ddc6cf
recv "00101ae42858fb419fb49d5613ddc6cf0803"(18)
INFO[0010] REC: {"cnt":3} streamUUID=00101ae4-2858-fb41-9fb4-9d5613ddc6cf
```

