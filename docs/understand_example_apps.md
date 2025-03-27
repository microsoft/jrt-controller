# Sample apps examples

There are a number of sample apps which can be follow.

## Simple Application

The __"simple_app"__ is a single application which reads data from a codelet's output channel, counts received messages, and once a counter is reached, sends the value to input channel of a different codelet.  

There are two versions of this simple example: a C version and a python version.  Both have identical behaviour, but have different programming models.

* [Simple example - C](./understand_simple_app_c.md)
* [Simple example - Python](./understand_simple_app_py.md)

## Advanced Application

The __"advanced_app"__ has two application. Both of these read and write from/to channels of codelets, but also send data between themselves.

There are two versions of this advanced example. 

* [Advanced example - C](./understand_advanced_app_c.md)
* [Advanced example - Python](./understand_advanced_app_py.md)
