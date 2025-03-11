# Sample apps examples

There are a number of sample apps which can be follow.

## Simple Application

The __"simple_app"__ is a single application which reads data from a codelet's output channel, counts received messages, and once a counter is reached, sends the value to input channel of a different codelet.  Full details of the logic see [Simple example](./understand_simple_app.md).

There are three versions of this simple example.  All three have identical behaviour, but have different programming models.

* [Simple example](./understand_simple_app.md)
This shows how to program in a "native" fashion.  In this mode the developer implements the channel initialisation, the receiver loop, freeing of resources etc.

* [Simple example - using JrtcApp abstraction class with C interface](./understand_simple_app_c.md)
In this version, the developer uses the JrtcApp class.  [See the API here](../src/wrapper_apis/c/jrtc_app.h).
In this model, the developer just needs to specify (i) the application state variables, (ii) the configuration of the streams, and (iii) a callback handler to deal with received messages or timeouts.

* [Simple example - using JrtcApp abstraction class with Python interface](./understand_simple_app_py.md)
This is identical to the previous version, but instead the application is written in Python, and uses a python wrapper.  See see [jrtc_app.py](../src/wrapper_apis/python/jrtc_app.py).

## Advanced Application

The __"advanced_app"__ has two application. Both of these read and write from/to channels of codelets, but also send data between themselves.
Full details of the logic see [Advanced example](./understand_advanced_app.md).

There are two versions of this advanced example.  Both have identical behaviour, but have different programming models.

* [Advanced example](./understand_advanced_app.md).
This is the "native" version. 

* [Advanced example - using JrtcApp abstraction class with C interface](./understand_advanced_app_c.md)
This is the C version using the JrtcApp abstraction class.
