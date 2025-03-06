
- [1. Understanding the simple example application (first\_example\_c)](#1-understanding-the-simple-example-application-first_example_c)
  - [1.1. Application behaviour](#11-application-behaviour)
  - [1.2. Implementation details](#12-implementation-details)
    - [1.2.1. Application state variables](#121-application-state-variables)
    - [1.2.2. Application configuration](#122-application-configuration)
    - [1.2.3. Callback handler](#123-callback-handler)
  - [1.3. Run the application](#13-run-the-application)
    - [1.3.1. Prerequisites](#131-prerequisites)
    - [1.3.2. Build the application](#132-build-the-application)
    - [1.3.3. Run the components](#133-run-the-components)
    - [1.3.4. Expected output](#134-expected-output)


# 1. Understanding the simple example application (first_example_c)

## 1.1. Application behaviour

The [first example](../sample_apps/first_example_c/) demonstrates the loading of a simple 
*jrt-controller* deployment.
A deployment in the *jrt-controller* terminology is a collection of modules that are loaded to the *jrt-controller* and the network functions as a set and operate as a single logical unit (i.e., as a single applicaction). 

A deployment can contain one or more application modules that are loaded to the *jrt-controller* and a set of codeletSets (collection of codelets) that are loaded to network functions instrumented with the jbpf framework. 
For details about the APIs and functionalities of the *jbpf* framework, please consult the *jbpf* [documentation](https://github.com/microsoft/jbpf/blob/main/README.md).

This example demonstrates a simple deployment that is composed of a single *jrt-controller* application and two codelets, which are loaded to a sample agent.
In this example, an agent corresponds to an instrumented network function. 

A [data_generator codelet](../sample_apps/jbpf_codelets/data_generator/data_generator_codelet.c) increments a counter and sends the data to the *jrt-controller* application via an output map called [ringbuf](../sample_apps/jbpf_codelets/data_generator/data_generator_codelet.yaml).
The *jrt-controller* [application](../sample_apps/first_example_c/first_example.c) aggregates the counter, prints a message about the aggregate value 
and sends this value to a [simple_input codelet](../sample_apps/jbpf_codelets/simple_input/simple_input_program.c) 
via a control_input channel called [input_map](../sample_apps/jbpf_codelets/simple_input/codeletset.yaml). 
The simple_input codelet prints the received values every time it is called by the agent.

To implement, Jrtc provides an (API)[../src/wrapper_apis/c/jrtc_app.h], which abstracts common functionality of all applications, such as the main execution loop, and resource management.

Using this API, the user just needs to define the following ...
- a structure detailing the app state variables.
- the application configuration, including the definitions of the streams which the app will use.
- a callback function which is called for every message received, or on the expiry of an inactivity timeout configured by the user.

Please the API here [API](../src/wrapper_apis/c/jrtc_app.h).

## 1.2. Implementation details

Here are the details given for the first_example.

### 1.2.1. Application state variables

The following shows the __AppStateVars_t__ which are the state variables for an app .

All apps must have the  __"JrtcApp* app;"__ field in the struct.   Individual applications may have additions fields as necessary.  In the case of the first_example, it has additional fields __"agg_cnt"__ and __"received_counter"__.

```C
typedef struct {
    JrtcApp* app;

    // add custom fields below
    int agg_cnt;
    int received_counter;
} AppStateVars_t;
```

### 1.2.2. Application configuration

The streams of the applications are defined as below. 

The __"stream routing information"__ defines where streams origination and destination.  In this example, the application makes a request to the *jrt-controller* router to receive data going to any destination (`JRTC_ROUTER_REQ_DEST_ANY`), from any network function registered to the controller (`JRTC_ROUTER_REQ_DEVICE_ID_ANY`).

The application also identifies the stream path, i.e. the creator of the stream, and the stream name. 
This information is defined in the application deployment descriptor file found [here](../sample_apps/jbpf_codelets/data_generator/data_generator_codelet.yaml).
The stream path is a string in the format `<deployment_name>://<source_path>`.
The name of the deployment in this example, as defined in the deployment descriptor file, is `FirstExample`.
Given that the stream of this example is created by a *jbpf* codelet, the source path name starts with the string `jbpf_agent` and is followed by the codeletset and codelet name, i.e., `jbpf_agent/data_generator_codeletset/codelet`.
Finally, the stream name is the name of the output map that the *jbpf* codelet uses to send the data out, i.e., `ringbuf` in this example.

The __"is_rx"__ field specifies whether, from this application's perspective, a channel is "rx" (i.e. the app will receive data from it), or it is "tx" (i.e. the app will send data to it)

The __"AppChannelCfg"__ field is used to create channels "owned" by the application itself.  These are used in the cases where JRTC applications send data to each other, as opposed to receiving/transmitting data from/to a Jbpf codelet.  These are not used in this first_example, so refer to [./understand_advanced_app_c.md](./understand_advanced_app_c.md) for further details.

```C
const JrtcStreamCfg_t streams[] = {
    // GENERATOR_OUT_SIDX
    { 
        { 
            JRTC_ROUTER_REQ_DEST_ANY, 
            JRTC_ROUTER_REQ_DEVICE_ID_ANY, 
            "FirstExample://jbpf_agent/data_generator_codeletset/codelet", 
            "ringbuf"
        },
        true,  // is_rx
        NULL   // No AppChannelCfg 
    },
    // SIMPLE_INPUT_IN_SIDX
    {
        { 
            JRTC_ROUTER_REQ_DEST_NONE, 
            jbpf_agent_device_id, 
            "FirstExample://jbpf_agent/simple_input_codeletset/codelet", 
            "input_map"
        },
        false, // is_rx
        NULL   // No AppChannelCfg
    }
};
```

The application config is detailed below.

```C
const JrtcAppCfg_t app_cfg = {
    "FirstExample",                            // context
    100,                                       // q_size
    sizeof(streams) / sizeof(streams[0]),      // num_streams
    (JrtcStreamCfg_t*)streams,                 // Pointer to the streams array
    10.0f,                                     // initialization_timeout_secs
    0.1f,                                      // sleep_timeout_secs
    1.0f                                       // inactivity_timeout_secs
};
```

where
```sh
context : A string passed to the abstraction class.  This is prepended to log messages to provide context.
q_size :  The number of elements in the data entry array which is used when the application class receives messages.
num_streams :  Number of streams.
streams : The streams defined above.
initialization_timeout_secs : Maximum time to allow for the initialisation to complete. 
                      If set to zero, no timer will be run.
sleep_timeout_secs :  How long the abstraction class receiver loop should sleep each iteration.  
                      This can be set to a nanosecond precision. 
                      If set to zero, no sleep will be done.
inactivity_timeout_secs : Inactivity duration to wait before callback the handler is called with "timeout=True".
                      If set to zero, no inactivity timer will be run.
```

### 1.2.3. Callback handler

```C
void app_handler(bool timeout, int stream_idx, jrtc_router_data_entry_t* data_entry, void* s) {

    AppStateVars_t* state = (AppStateVars_t*)s;

    if (timeout) {
        // Timeout processing (not implemented in this example)
    } else {

        if (stream_idx == GENERATOR_OUT_SIDX) {
            state->received_counter += 1;

            // Extract data from the received entry
            example_msg* data = (example_msg*)data_entry->data;
            state->agg_cnt += data->cnt;
        }

        if (state->received_counter % 5 == 0 && state->received_counter > 0) {

            simple_input aggregate_counter = {0};

            aggregate_counter.aggregate_counter = state->agg_cnt;

            jrtc_router_stream_id_t sid = jrtc_app_get_stream(state->app, SIMPLE_INPUT_IN_SIDX);

            int res = jrtc_router_channel_send_input_msg(sid, &aggregate_counter, sizeof(aggregate_counter));
            assert(res == 0 && "Failure returned from jrtc_router_channel_send_input_msg");

            printf("FirstExample: Aggregate counter so far is: %u \n", state->agg_cnt);
         }
    }
}
```

The callback handler is explained below.

If the inactivity timer expires, the handler will be called with __timeout__ set to True.  All other arguments will be NULL and should NOT be accessed in the path which deals with the timeout.

In the cases where the handler is called when data arrives, the other arguments are ...
```sh
stream_index : The stream index of the stream on which the message has been received.  The index is zero-based.  In this case, stream index "0" means it has been received on stream "FirstExample://jbpf_agent/data_generator_codeletset/codelet"/"ringbuf".
data_entry: the data message itself.
s: an opaque pointer to the AppStateVars_t.  This should be cast to "state"
```

It can be seen that in this simple handler, "agg_cnt" is incremented by 1 for every message received from "FirstExample://jbpf_agent/data_generator_codeletset/codelet"/"ringbuf".

If the number of received messages is divisible by 5, the handler creates an "aggregate_counter" struct, populates it with agg_cnt, and sends it to "FirstExample://jbpf_agent/simple_input_codeletset/codelet"/"input_map" by calling  "jrtc_app_router_channel_send_input_msg".

## 1.3. Run the application

### 1.3.1. Prerequisites

Before running the sample apps, the *jrt-controller* must be built (see [README.md](../../README.md) for instructions).

### 1.3.2. Build the application

The application can be built using the following commands:
  ```sh
  cd $JRTC_PATH/sample_apps/first_example_c
  make
  ```

This will build the two sample codelets (`data_generator` and `simple_input`), the sample agent (`simple_agent_ipc`) and the *jrt-controller* application (`app1.so`).


### 1.3.3. Run the components

You will need to open five terminals.

* **Terminal 1:** 
  
  This is used to run the *jrt-controller*. 
  ```sh
  cd $JRTC_PATH/sample_apps/first_example_c
  ./run_jrtc.sh
  ```

* **Terminal 2:**

  This is used to run the test agent, where the example codelets will be loaded.
  ```sh
  cd $JRTC_PATH/sample_apps/first_example_c
  ./run_simple_agent_ipc.sh
  ```
  If successful, one of the log messages at the agent side should report `Registration succeeded`.

* **Terminal 3:**

  This is used to run the *jbpf* [reverse proxy](https://github.com/microsoft/jbpf/blob/main/docs/life_cycle_management.md) process, which enables the loading of codelets to the agent using a TCP socket (e.g., from a remote machine).

  ```sh
  cd $JRTC_PATH/sample_apps/first_example_c
  ./run_reverse_proxy.sh
  ```

* **Terminal 4:**

  This is used to run the *jrt-controller" decoder.
  
  ```sh
  cd $JRTC_PATH/sample_apps/first_example_c
  ./run_decoder.sh
  ```

* **Terminal 5:**

  This is used to load and unload the example application.
  ```sh
  cd $JRTC_PATH/sample_apps/first_example_c
  ./load_app.sh
  ```


### 1.3.4. Expected output

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

