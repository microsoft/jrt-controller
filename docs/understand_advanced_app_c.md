
- [1. Understanding the advanced example application (advanced\_example\_c)](#1-understanding-the-advanced-example-application-advanced_example_c)
  - [1.1. Implementation details - AdvancedExample1](#11-implementation-details---advancedexample1)
    - [1.1.1. Application state variables](#111-application-state-variables)
    - [1.1.2. Application configuration](#112-application-configuration)
    - [1.1.3. Callback handler](#113-callback-handler)
  - [1.2. Implementation details - AdvancedExample2](#12-implementation-details---advancedexample2)
    - [1.2.1. Application state variables](#121-application-state-variables)
    - [1.2.2. Application configuration](#122-application-configuration)
    - [1.2.3. Callback handler](#123-callback-handler)
  - [1.3. Run the application](#13-run-the-application)
    - [1.3.1. Prerequisites](#131-prerequisites)
    - [1.3.2. Build the application](#132-build-the-application)
    - [1.3.3. Run the components](#133-run-the-components)
    - [1.3.4. Expected output](#134-expected-output)


# 1. Understanding the advanced example application (advanced_example_c)

The logic of [advanced example c](../sample_apps/advanced_example_c/) is identical to the [advanced example](../sample_apps/advanced_example/).

To see the details of this logic and expected behaviour, refer to [./understand_advanced_app.md](./understand_advanced_app.md)

The difference between the two is that advanced_example_c uses the JrtcApp abstraction class.  [See the API here](../src/wrapper_apis/c/jrtc_app.h).  To see details of the JrtApp API, refer to [./understand_simple_app_c.md](./understand_simple_app_c.md)

## 1.1. Implementation details - AdvancedExample1

Here are the details given for the advanced_example. 

### 1.1.1. Application state variables

In the case of the advanced_example1, it has additional field __"agg_cnt"__.

```C
typedef struct {
    JrtcApp* app;

    // add custom fields below
    int agg_cnt;
} AppStateVars_t;
```

### 1.1.2. Application configuration

The streams of the applications are defined as below. It can be seen that these are as per described in [./understand_advanced_app.md](./understand_advanced_app.md).

```C
const JrtcStreamCfg_t streams[] = {
    // GENERATOR_OUT_SIDX
    {
        {
            JRTC_ROUTER_REQ_DEST_ANY, 
            JRTC_ROUTER_REQ_DEVICE_ID_ANY, 
            "AdvancedExample1://jbpf_agent/data_generator_codeletset/codelet", 
            "ringbuf"
        },
        true,  // is_rx
        NULL   // No AppChannelCfg 
    },
    // APP2_OUT_SIDX
    {
        {
            JRTC_ROUTER_REQ_DEST_ANY, 
            jrtc_device_id, 
            "AdvancedExample2://intraapp", 
            "buffer"
        },
        true,  // is_rx
        NULL   // No AppChannelCfg 
    },
    // SIMPLE_INPUT_CODELET_IN_SIDX
    {
        {
            JRTC_ROUTER_REQ_DEST_NONE, 
            jbpf_agent_device_id, 
            "AdvancedExample1://jbpf_agent/simple_input_pb_codeletset/codelet", 
            "input_map"
        },
        false,  // is_rx
        NULL   // No AppChannelCfg 
    },
    // APP1_IN_SIDX
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
};
```

In this example, the 4th stream also specifies that an "application" channel must be created.  Here are fields which are set ..
```sh
is_output: false  // Indicates if the channel is for output
num_elems: 100   // Number of elements in the channel
elem_size: sizeof(simple_input_pb)   // Size of each element in bytes
```

This is specifically creating an "input" channel for the AdvancedExample1 app.

The application config is detailed below.

```C
const JrtcAppCfg_t app_cfg = {
    "AdvancedExample1",                            // context
    100,                                       // q_size
    sizeof(streams) / sizeof(streams[0]),      // num_streams
    (JrtcStreamCfg_t*)streams,                 // Pointer to the streams array
    0.1f,                                      // sleep_timeout_secs
    1.0f                                       // inactivity_timeout_secs
};
```

### 1.1.3. Callback handler

```C
void app_handler(bool timeout, int stream_idx, jrtc_router_data_entry_t* data_entry, void* s) {

    AppStateVars_t* state = (AppStateVars_t*)s;

    if (timeout) {
        // Timeout processing (not implemented in this example)
    } else {

        switch (stream_idx) {

            case GENERATOR_OUT_SIDX:
            {
                // Extract data from the received entry
                example_msg* data = (example_msg*)data_entry->data;
                state->agg_cnt += data->cnt;
                printf("App1: Aggregate counter from codelet is %d\n", state->agg_cnt);

                // Send the aggregate counter back to the input codelet
                simple_input_pb aggregate_counter = {0};
                jrtc_router_stream_id_t sid = jrtc_app_get_stream(state->app, SIMPLE_INPUT_CODELET_IN_SIDX);
                int res = jrtc_router_channel_send_input_msg(sid, &state->agg_cnt, sizeof(aggregate_counter));
                assert(res == 0 && "Failure in jrtc_router_channel_send_input_msg");
            }
            break;
                
            case APP2_OUT_SIDX:
            {
                // Data received from App2 output channel
                simple_input_pb* appdata = (simple_input_pb*)data_entry->data;
                printf(
                    "App1: Received aggregate counter %d from output channel of App2\n",
                    appdata->aggregate_counter);
            }
            break;

            case APP1_IN_SIDX:
            {
                // Data received from App1 input channel (by App2)
                simple_input_pb* appdata = (simple_input_pb*)data_entry->data;
                printf(
                    "App1: Received aggregate counter %d from input channel of App1\n", 
                    appdata->aggregate_counter);

            }
            break;

            default:
                printf("App1: Got some unexpected message\n");
                assert(false);
        }
    }
}
```

## 1.2. Implementation details - AdvancedExample2

Here are the details given for the advanced_example. 

### 1.2.1. Application state variables

In the case of the advanced_example1, it has additional fields __"agg_cnt"__ and __"received_counter"__.

```C
typedef struct {
    JrtcApp* app;

    // add custom fields below
    int agg_cnt;
    int received_counter;
} AppStateVars_t;
```

### 1.2.2. Application configuration

The streams of the applications are defined as below. It can be seen that these are as per described in [./understand_advanced_app.md](./understand_advanced_app.md).

```C
    const JrtcStreamCfg_t streams[] = {
        // GENERATOR_PB_OUT_SIDX = 0,
        {
            {
                JRTC_ROUTER_REQ_DEST_ANY, 
                JRTC_ROUTER_REQ_DEVICE_ID_ANY, 
                "AdvancedExample2://jbpf_agent/data_generator_pb_codeletset/codelet", 
                "ringbuf"
            },
            true,  // is_rx
            NULL   // No AppChannelCfg 
        },
        // APP2_OUT_SIDX
        {
            {
                JRTC_ROUTER_REQ_DEST_NONE, 
                jrtc_device_id, 
                "AdvancedExample2://intraapp", 
                "buffer"
            },
            false,   // is_rx
            &(JrtcAppChannelCfg_t){true, 100, sizeof(simple_input_pb)}
        },
        // APP1_IN_SIDX
        {
            {
                JRTC_ROUTER_REQ_DEST_NONE, 
                jrtc_device_id, 
                "AdvancedExample1://intraapp", 
                "buffer_input"
            },
            false,  // is_rx
            NULL   // No AppChannelCfg 
        }
    };
```

In this example, the 4nd stream also specifies that an "application" channel must be created.  Here are fields which are set ..
```sh
is_output: true  // Indicates if the channel is for output
num_elems: 100   // Number of elements in the channel
elem_size: sizeof(simple_input_pb)   // Size of each element in bytes
```
This is specifically creating an "output" channel for the AdvancedExample2 app.

The application config is detailed below.

```C
const JrtcAppCfg_t app_cfg = {
    "AdvancedExample2",                        // context
    100,                                       // q_size
    sizeof(streams) / sizeof(streams[0]),      // num_streams
    (JrtcStreamCfg_t*)streams,                 // Pointer to the streams array
    0.1f,                                      // sleep_timeout_secs
    1.0f                                       // inactivity_timeout_secs
};
```

### 1.2.3. Callback handler

```C
// Function for handling received data (as in Python)
void app_handler(bool timeout, int stream_idx, jrtc_router_data_entry_t* data_entry, void* s) {

    AppStateVars_t* state = (AppStateVars_t*)s;

    if (timeout) {
        // Timeout processing (not implemented in this example)
    } else {

        switch (stream_idx) {

            case GENERATOR_PB_OUT_SIDX:
            {
                state->received_counter++;
                example_msg_pb* data = (example_msg_pb*)data_entry->data;
                state->agg_cnt += data->cnt;

                printf("App2: Aggregate counter from codelet is %d\n", state->agg_cnt);

                // Get a buffer to write the output data
                dapp_channel_ctx_t chan_ctx = jrtc_app_get_channel_context(state->app, APP2_OUT_SIDX);
                simple_input_pb* counter = (simple_input_pb*)jrtc_router_channel_reserve_buf(chan_ctx);
                assert(counter);
                counter->aggregate_counter = state->agg_cnt;
                // Send the data to the App2 output channel
                int res = jrtc_router_channel_send_output(chan_ctx);
                assert(res == 0 && "Error returned from jrtc_router_channel_send_output");

                // Send the data to the App1 input channel
                simple_input_pb aggregate_counter = {0};
                aggregate_counter.aggregate_counter = state->agg_cnt;

                jrtc_router_stream_id_t stream = jrtc_app_get_stream(state->app, APP1_IN_SIDX);
                res = jrtc_router_channel_send_input_msg(
                    stream, &aggregate_counter, sizeof(aggregate_counter));
                assert(res == 0 && "Failure in jrtc_router_channel_send_input_msg");
            }
            break;
                
            default:
                printf("App1: Got some unexpected message\n");
                assert(false);
        }

        if (state->received_counter % 5 == 0 && state->received_counter > 0) {
            printf("App2: Aggregate counter so far is %u\n", state->agg_cnt);
            // flush to standard output
            fflush(stdout);
        }        
    }
}
```

In the above handler it can be seen the app will send to its own "output" channel, which is APP2_OUT_SIDX.

However to do that, it first calls helper function __"jrtc_app_get_channel_context"__.  This is a helper function provided by the JrtcApp class, which returns a Jrtc channel context structure for a given stream index.  This stream structure is then used when calling the core Jrtc function "jrtc_router_channel_send_output".

## 1.3. Run the application

### 1.3.1. Prerequisites

Before running the sample apps, the *jrt-controller* must be built (see [README.md](../../README.md) for instructions).

### 1.3.2. Build the application

The application can be built using the following commands:
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  make
  ```

This will build the all the sample codelets (`data_generator`, `data_generator_protobuf`, `simple_input` and `simple_input_protobuf`), the sample agent (`simple_agent_ipc`) and the *jrt-controller* application (`app1.so`).


### 1.3.3. Run the components

You will need to open five terminals.

* **Terminal 1:** 
  
  This is used to run the *jrt-controller*. 
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  ./run_jrtc.sh
  ```

* **Terminal 2:**

  This is used to run the test agent, where the example codelets will be loaded.
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  ./run_simple_agent_ipc.sh
  ```
  If successful, one of the log messages at the agent side should report `Registration succeeded`.

* **Terminal 3:**

  This is used to run the *jbpf* [reverse proxy](https://github.com/microsoft/jbpf/blob/main/docs/life_cycle_management.md) process, which enables the loading of codelets to the agent using a TCP socket (e.g., from a remote machine).

  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  ./run_reverse_proxy.sh
  ```

* **Terminal 4:**

  This is used to run the *jrt-controller" decoder.
  
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  ./run_decoder.sh
  ```

* **Terminal 5:**

  This is used to load and unload the example application.
  ```sh
  cd $JRTC_PATH/sample_apps/advanced_example_c
  ./load_app.sh
  ```


### 1.3.4. Expected output

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

