# jrtctl command line interface

[jrtc-ctl](../tools/jrtc-ctl/README.md) is a command line interface tool to load/unload a jrt-controller bundles made up from *jbpf* codelets and *jrt-controller* applications.
It acts as a thin layer that bundles all the APIs required to manage the different components:

* *jbpf*: The tool will call *jbpf* REST APIs through the reverse proxy to load a codelet in the *jbpf* agent from a predefined storage location. See the [jbpf documentation](https://github.com/microsoft/jbpf) for more details. 

* *jrt-controller*: The tool will use the *jrt-controller* REST API to load a deployment and its applications. 


If serialization is required, *jrt-controller* will use the schema's context to provide access to the de/serialization functions. *jrtc-ctl* will load the schema into two places: 

* *jbpf*: The schema is stored with the stream context in *libjbpf_io*, and it is loaded at the stream creation time through *jbpf* API. 

* *Data collection and control* end-point: The end-point provides an interface (e.g., grpc) to load the de/serialization functions. 


This is illustrated in the high-level architecture diagram shown on the figure below. 
For more information on the forwarding application and the decoder, see [here](./streams.md).
For more information on how to use it, see [here](../tools/jrtc-ctl/README.md).

![architecture](./jrtcontroller_architecture.png)

## Other related tools

* [jrtcjsonschema](../tools/jrtcjsonschema) is a utility tool used to generate json schemas and output them to `./schemas` directory. It is called from the `jrtc-ctl` tool in a generation step.
* [schemas](../tools/schemas) contains json schemas to statically validate jrt-controller bundle configurations.



