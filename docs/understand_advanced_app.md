- [1. Understanding the advanced example application](#1-understanding-the-advanced-example-application)
  - [1.1. Application description](#11-application-description)
    - [1.1.1. Automatic serialization and routing](#111-automatic-serialization-and-routing)
    - [1.1.2. Messages between applications](#112-messages-between-applications)
  - [1.2. Run the application](#12-run-the-application)
    - [1.2.1. Prerequisites](#121-prerequisites)
    - [1.2.2. Build the application](#122-build-the-application)
    - [1.2.3. Run the example](#123-run-the-example)
    - [1.2.4. Expected output](#124-expected-output)


# 1. Understanding the advanced example application

This [advanced example](../sample_apps/advanced_example/) builds on the basic concepts introduced in the [simple example](./understand_simple_app.md) and demonstrates more advanced capabilities offered by the *jrt-controller*.


## 1.1. Application description

This example involves two deployments.

The [first](../sample_apps/advanced_example/advanced_example1.yaml), called `AdvancedExample1`, consists of one *jrt-controller* application `app1` and two codelets:

* The [data_generator codelet](../sample_apps/advanced_example/jbpf_codelets/data_generator/data_generator_codelet.c) increments a counter every time it is called and sends it to `app1`. 
  It is the same as the one in the simple example. 

* The [simple_input codelet](../sample_apps/advanced_example/jbpf_codelets/simple_input/simple_input_program.c) receives an input from the *jrt-controller* application and prints it on the console. 
  It is the same as the one in the simple example. 

The [second](../sample_apps/advanced_example/advanced_example2.yaml) deployment, called `AdvancedExample2`, consists of a *jrt-controller* application `app2` and a single codelet:

* The [data_generator_protobuf codelet](../sample_apps/advanced_example/jbpf_codelets/data_generator_protobuf/data_generator_pb_codelet.c) is the same as the *data_generator codelet* of `app1`, except that it uses a [protobuf schema](../sample_apps/advanced_example/jbpf_codelets/data_generator_protobuf/generated_data.proto) for exposing the data to the *jrt-controller*.
This enables the serialized data forwarding to a *Data collection and control* point, in addition to exposing the data to  `app2`.



The deployments have the following application logic:
* [app1](../sample_apps/advanced_example/advanced_example1.c) reads data from the *data_generator* codelet, updates a counter, and sends the result back to the *simple_input* codelet. This part is the same as in the [simple_example app](../sample_apps/first_example/first_example.c). 
  `app1` also receives data from `app2` over two streams (one input and one output) and prints it on screen. 

* [app2](../sample_apps/advanced_example/advanced_example1.c) reads data from the `data_generator_protobuf` codelet, updates a counter, and sends the updated counters to `app2` over both the input and output streams. 

There are several new elements illustrated in the advanced example that we discuss in more details. 


### 1.1.1. Automatic serialization and routing

One new element is the [data_generator_protobuf codelet](../sample_apps/advanced_example/jbpf_codelets/data_generator_protobuf/data_generator_pb_codelet.c) of deployment `AdvancedExample2`, which introduces the concept of automatic data serialization. 
Instead of manually defining a C struct for the data sent to `app2` through the output stream `ringbuf`, the codelet leverages a protobuf schema definition, which allows the same data to also be sent to one or more *Data collection and control* end-points over the network in a serialized format (protobuf). 
The protobuf schema that is employed for this example can be found [here](../sample_apps/advanced_example/jbpf_codelets/simple_input/simple_input.proto).


When [building](../sample_apps/advanced_example/jbpf_codelets/data_generator_protobuf/Makefile) the binaries of the deployment `AdvancedExample2` using `make`, a file called `generated_data.pb.h` is auto-generated, containing a C struct called `example_msg`.
The build process also generates a protobuf descriptor called (`generated_data.pb`) and a serde (serialization/deserialization) library called (`example_msg_serializer.so`).


The codelet can use the `example_msg` structure to populate the output data, just as any other user-defined C struct:
```C
out = (example_msg*)c;
out->cnt = cnt;
jbpf_ringbuf_output(&ringbuf, out, sizeof(example_msg));
```
When the message is sent to the output channel by the jbpf codelet using the `jbpf_ringbuf_output()` API call, it is received by the *jrt-controller* applications (`app2` in this case) without any serialization overhead (plain memory and with zero-copy), just as in the simple example.

However, in the case of this advanced example, the deployment also forwards the message to a remote UDP socket end-point by specifying a [forwarding policy](../sample_apps/advanced_example/jbpf_codelets/data_generator_protobuf/data_generator_pb_codelet.yaml) in the codelet descriptor file:

```
out_io_channel:
  - name: ringbuf
    forward_destination: DestinationUDP
    serde:
      file_path: ${JRTC_PATH}/sample_apps/advanced_example/jbpf_codelets/data_generator_protobuf/generated_data:example_msg_serializer.so
    protobuf:
      package_path: ${JRTC_PATH}/sample_apps/advanced_example/jbpf_codelets/data_generator_protobuf/generated_data.pb
      msg_name: example_msg
```
Here, the descriptor file specifies that any data sent through the output stream with name `ringbuf` should be routed to the *Data collection and control* end-point called `DestinationUDP` (the UDP socket).
When forwarded, the data should be serialized using the protobuf descriptor and serde library specified under the `serde` and `protobuf` sections (generated through the build process explained previously).

The *jrt-controller* is bundled with a [forwarding application](../src/controller/jrtc_north_io_app.c), which is always enabled and subscribed to all streams with forwardng destination `JRTC_ROUTER_DEST_UDP` (the `DestinationUDP` end-point).
The forwarding application fetches data for any relevant stream, serializes it using the provided serialization information for the stream and sends it to the destination UDP socket over the network.
The destination IP and UDP port used by the forwarding application are defined through the env variables `AA_NET_OUT_FQDN` and `AA_NET_OUT_PORT` correspondingly.

Users can implement their own UDP end-point to handle the output data sent by the fowrading application (e.g., as part of their SMO), but *jrt-controller* also bundles a basic end-point with the [jrtc-ctl](jrtctl.md) tool, which can be useful for testing purposes.
The end-point, called `jrtc-ctl decoder`, simply receives the data via a UDP port, deserializes them, and prints them on screen in JSON format.

### 1.1.2. Messages between applications

In the previous example, a *jbpf* codelet was the creator of each channel. 
*jrt-controller* applications can also communicate amongst themselves using channels.
As with codelets, there are two types of application channels, *input* and *output* (see [here](../docs/streams.md) for more details).

For example, *app2* creates an instance of an output channel using the following calls:
```C
    // Get the stream ID for the control input codelet
    jrtc_router_generate_stream_id(
        &stream_id_app_output,
        JRTC_ROUTER_REQ_DEST_NONE,
        jrtc_device_id,
        "AdvancedExample2://intraapp",
        "buffer");

    dapp_channel_ctx_t ctx = jrtc_router_channel_create(env_ctx->dapp_ctx, 
        true /*is_output*/, 100, sizeof(app_msg), stream_id_app_output, NULL /*descriptor*/, 0 /*descriptor_size*/);
```
As with the case of codelets, `app1` subscribes to this stream to receive data produced by `app2`.

The example also demonstrates how the same can be done for input channels, with `app1` creating the channel and `app2` sending data to it.

Note that the channel creation can optionally support de/serialization function if the data needs to be exported to external end-points. 


## 1.2. Run the application

### 1.2.1. Prerequisites

Before running the sample apps, the *jrt-controller* must be built (see [README.md](../../README.md) for instructions).


### 1.2.2. Build the application

To build the application, type:
```sh
cd $JRTC_PATH/sample_apps/advanced_example
make
```

### 1.2.3. Run the example

You will need to open five terminals:
* **Terminal 1:** 
  
  This is used to run the *jrt-controller*. 
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example
  ./run_jrtc.sh
  ```

* **Terminal 2:**

  This is used to run the test agent, where the example codelets will be loaded.
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example
  ./run_simple_agent_ipc.sh
  ```
  If successful, one of the log messages at the agent side should report `Registration succeeded`.

* **Terminal 3:**

  This is used to run the *jbpf* [reverse proxy](https://github.com/microsoft/jbpf/blob/main/docs/life_cycle_management.md) process, which enables the loading of codelets to the agent using a TCP socket (e.g., from a remote machine).

  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example
  ./run_reverse_proxy.sh
  ```

* **Terminal 4:** 

  This is used to load the UDP decoder end-point:
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example
  ./load_decoder.sh
  ```

* **Terminal 5:**

  This is used to load and unload the example application.
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example
  ./load_app.sh
  ```



### 1.2.4. Expected output

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


The application can then be unloaded with the following command:
```sh
./unload_app.sh
```

