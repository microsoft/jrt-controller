# Understanding the simple example application

The [first example](../sample_apps/first_example/) demonstrates the loading of a simple *jrt-controller* deployment.
A deployment in the *jrt-controller* terminology is a collection of modules that are loaded to the *jrt-controller* and the network functions as a set and operate as a single logical unit (i.e., as a single applicaction). 

A deployment can contain one or more application modules that are loaded to the *jrt-controller* and a set of codeletsets (collection of codelets) that are loaded to network functions instrumented with the jbpf framework. 
For details about the APIs and functionalities of the *jbpf* framework, please consult the *jbpf* [documentation](https://github.com/microsoft/jbpf/blob/main/README.md).

## Application description

This example demonstrates a simple deployment that is composed of a single *jrt-controller* application and two codelets, which are loaded to a sample agent.
In this example, an agent corresponds to an instrumented network function. 

A [data_generator codelet](../sample_apps/first_example/jbpf_codelets/data_generator/data_generator_codelet.c) increments a counter and sends the data to the *jrt-controller* application via an output map called [ringbuf](../sample_apps/first_example/jbpf_codelets/data_generator/data_generator_codelet.yaml).
The *jrt-controller* [application](../sample_apps/first_example/first_example.c) aggregates the counter, prints a message about the aggregate value 
and sends this value to a [simple_input codelet](../sample_apps/first_example/jbpf_codelets/simple_input/simple_input_program.c) 
via a control_input channel called [input_map](../sample_apps/first_example/jbpf_codelets/simple_input/codeletset.yaml). 
The simple_input codelet prints the received values everytime it is called by the agent.

## Implementation details

### Receiving data from *jbpf* codelets

To receive the data from the data_generator codelet, the application must first subscribe to the codelet's output stream. This is done though the following API calls:
```C
jrtc_router_generate_stream_id(
  &codelet_sid,
  JRTC_ROUTER_REQ_DEST_ANY,
  JRTC_ROUTER_REQ_DEVICE_ID_ANY,
  "FirstExample://jbpf_agent/data_generator_codeletset/codelet1",
"ringbuf");

jrtc_router_channel_register_stream_id_req(env_ctx->dapp_ctx, codelet_sid);
```
First, the application must create a unique 16 bytes stream identifier `codelet_sid` by calling the function `jrtc_router_generate_stream_id` (see [here](./streams.md) for more info on streams and messages).

The stream id generation function takes as arguments the relevant routing infomation. In this example, the application makes a request to the *jrt-controller* router to receive data going to any destination (`JRTC_ROUTER_REQ_DEST_ANY`), from any network function registered to the controller (`JRTC_ROUTER_REQ_DEVICE_ID_ANY`).

The application also identifies the stream path, i.e., the creator of the stream, and the stream name. 
This information is defined in the application deployment descriptor file found [here](../sample_apps/first_example/jbpf_codelets/data_generator/data_generator_codelet.yaml).
The stream path is a string in the format `<deployment_name>://<source_path>`.
The name of the deployment in this example, as defined in the deployment descriptor file, is `FirstExample`.
Given that the stream of this example is created by a *jbpf* codelet, the source path name starts with the string `jbpf_agent` and is followed by the codeletset and codelet name, i.e., `jbpf_agent/data_generator_codeletset/codelet1`.
Finally, the stream name is the name of the output map that the *jbpf* codelet uses to send the data out, i.e., `ringbuf` in this example.

Using the generated stream_id, the application can subscribe to the codelet's stream using the `jrtc_router_channel_register_stream_id_req()` API call.

Once the application subscribes to the desired stream, it can start receiving data in the following way:

```C
num_rcv = jrtc_router_receive(env_ctx->dapp_ctx, data_entries, DATA_ENTRIES_SIZE);
if (num_rcv > 0) {
  for (int i = 0; i < num_rcv; i++) {
    if (jrtc_router_stream_id_matches_req(&data_entries[i].stream_id, &codelet_sid)) {
    received_counter++;
    data = data_entries[i].data;
    agg_cnt += data->cnt;
  }
  jrtc_router_channel_release_buf(data);
}
```

The call `jrtc_router_receive()` returns up to `DATA_ENTRIES_SIZE` data entries received from any source the application has subscribed to.
The application then checks the origin of the received entry `jrtc_router_stream_id_matches_req()` and handles the received data accordingly.

To minimize the communication latency, the data entries are distributed to the applications in a zero copy manner.
As such, the application must also make sure to release the received data buffer, after it no longer requires it, by calling `jrtc_router_channel_release_buf()`.

### Sending input data to *jbpf* codelets

Similarly to receiving data, the application must generate a stream id handler `control_input_sid` for sending control input data to the `simple_input` codelet:
```C
jrtc_router_generate_stream_id(
  &control_input_sid,
  JRTC_ROUTER_REQ_DEST_NONE,
  jbpf_agent_device_id,
  "FirstExample://jbpf_agent/unique_id_for_codelet_simple_input/codelet1",
  "input_map");
```
For this, it specifies that the stream id will not be used to route the sent messages to any external entities (`JRTC_ROUTER_REQ_DEST_NONE`), as well as the unique id of the jbpf_agent where the destination simple_input codelet is loaded (`jbpf_agent_device_id`).
The stream is destined to a *jbpf* codelet `codelet1` that is part of the codeletset `unique_id_for_codelet_simple_input`.
As such, the stream path becomes `FirstExample://jbpf_agent/unique_id_for_codelet_simple_input/codelet1`.
Finally, the stream name of the input map, where the codelet expects the input data is `input_map`.

To send to the input map of the codelet, the application uses the generated stream id and calls:
```C
jrtc_router_channel_send_input_msg(control_input_sid, &aggregate_counter, sizeof(aggregate_counter));
```
where `aggregate_counter` is the structure that is expected as input by the simple_input codelet. 

## Run the application

### Prerequisites

Before running the sample apps, the *jrt-controller* must be built (see [README.md](../../README.md) for instructions).


### Build the application

The application can be built using the following commands:
  ```sh
  cd $JRTC_PATH/sample_apps/first_example
  make
  ```

This will build the two sample codelets (`data_generator` and `simple_input`), the sample agent (`simple_agent_ipc`) and the *jrt-controller* application (`app1.so`).


### Run the components

You will need to open four terminals.

* **Terminal 1:** 
  
  This is used to run the *jrt-controller*. 
  ```sh
  cd $JRTC_PATH/sample_apps/first_example
  ./run_jrtc.sh
  ```

* **Terminal 2:**

  This is used to run the test agent, where the example codelets will be loaded.
  ```sh
  cd $JRTC_PATH/sample_apps/first_example
  ./run_simple_agent_ipc.sh
  ```
  If successful, one of the log messages at the agent side should report `Registration succeeded`.

* **Terminal 3:**

  This is used to run the *jbpf* [reverse proxy](https://github.com/microsoft/jbpf/blob/main/docs/life_cycle_management.md) process, which enables the loading of codelets to the agent using a TCP socket (e.g., from a remote machine).

  ```sh
  cd $JRTC_PATH/sample_apps/first_example
  ./run_reverse_proxy.sh
  ```

* **Terminal 4:**

  This is used to load and unload the example application.
  ```sh
  cd $JRTC_PATH/sample_apps/first_example
  ./load_app.sh
  ```


### Expected output

If the codelets and the app were loaded successfully, you should see the following output at the jrt-controller:
```
App 1: Aggregate counter so far is 15
App 1: Aggregate counter so far is 55
...
```

Similarly, you should see the following printed messages on the agent side from the simple input codelet:
```
[JBPF_DEBUG]: Got aggregate value 15
[JBPF_DEBUG]: Got aggregate value 55
...
```

The application can then be unloaded with the following command:
```sh
./unload_app.sh
```

