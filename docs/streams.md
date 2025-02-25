# Streams and Messages


## Streams 

A *jrt-controller* stream is created by a *jrt-controller* application or a *jbpf* codelet. 
It is uniquely identfied by a 16 bytes stream ID (defined in `struct jrtc_router_stream_id_int` in [this file](../src/stream_id/jrtc_router_stream_id.h)). 
The stream ID consists of the following fields:

* *ver* (6 bits) denotes a software version ID

* *fwd_dst* (7 bits) represent the forward destination in the *Data collection and control* components (see [high-level architecture diagram](./overview.md)). It allows the stream creator to specify the end destination so a generic routing application can forward multiple data streams. Note that the *jrt-controller* imposes no semantics on the values of the field. 
Higher-level applications can overload it appropriately where needed.

* *device_id* (7 bits) is a unique identifier of a network function that created the stream. This helps applications differentiate between multiple network functions connected to the same *jrt-controller* instance. The controller imposes no semantic on the values (apart from 0 being reserved for the controller), they are simply used as a discriminator for routing. The other device IDs are allocated in the application YAML descriptor. 

* *stream_path* (54 bits) identifies the creator of the stream. If a stream is created by *jrt-controller*, it is a hash of the *jrt-controller* application name. If a stream is created by *jbpf*, it is a hash of concatenation of the [codelet set](https://github.com/microsoft/jbpf/blob/main/docs/understand_first_codelet.md#codelet-load-request) and the codelet names:
  ```
  <jrtc_deployment_name>://jbpf_agent/<codeletset_name>/<codelet_name>
  ```

* *stream_name* (54 bits) identifies the streams from the same creator. It can have an arbitrary value for *jrt-controller* applications created streams. It is set to the [out_io_channel](https://github.com/microsoft/jbpf/blob/main/docs/understand_first_codelet.md#codelet-load-request) name if created by *jbpf*. 

Note that *jbpf* uses 16B stream IDs but has no semantics in them. This is why *jbpf* examples specify stream IDs in the channel definitions. 
The *jrt-ctl* assigns the appropriate stream IDs according to the above semantics at the application load time. 



### Input vs output streams

There is a different between output and input streams. 
An output stream has one producer who creates the stream and sends data to it. There can be multiple consumers of that stream, implementing 1:N communication model. One example is *jbpf* codelet sending telemetry data. Multiple applications can subscribe to it.

An input stream has one consumer who also creates the stream, but can have multiple producers. It implements N:1 communication model. One example is the *jbpf* control codelet in which multiple applications can send commands to one codelet. 

To see examples of input and output streams and how to create them, see the [advanced example](./understand_advanced_app.md).




## Messages

The *jrt-controller* uses the [jbpf-io](https://github.com/microsoft/jbpf/tree/main/src/io) library for sending messages. 
This library implements messaging over a shared memory with zero copy in order to reduce the latency. 
The most common pattern is to have the messages containing pointers to buffers of memory used by the *jbpf* codelet or the even the network function itself. 
In this case a *jrt-controller* application needs to cast the received message to the same memory layout (e.g. C structure) and is able to use the message directly. 

In some cases, such as when the messages are directly routed over the network to an external *Data collection and control* component, it is useful to serialize the message. 
This is discussed next. 



### Routing to external entitites

The *jrt-controller* comes with a default forwarding application that forwards all messages to an external *Data collection and control* end-point. 
One common use case of this feature is to forward *jbpf* data to the end-point without needing to write a *jrt-controller* application (this is discussed in the [advanced example](./understand_advanced_app.md)). 

One can use the *fwd_dst* to build a generic routing application that forwards to another *Data collection and control* end-point using a different protocols (such as E2 for a nRT-RIC or O1 for an SMO). 

In the case of external *Data collection and control* end-points the forwarded data has to be transmitted over the network, hence they have to be serialized. The *jrt-controller* uses the protobuf serializer from [jbpf_protobuf library](https://github.com/microsoft/jbpf-protobuf) to achieve this. The *jrtctl* loads the protobuf descriptor to the default forwarding application (for serialization) and to the *Data collection and control* end-point (for deserialization) using the grpc protocol. 
See the the [advanced example](./understand_advanced_app.md) for details. 









