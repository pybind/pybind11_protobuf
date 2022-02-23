# Pybind11 bindings for Google's Protocol Buffers

[TOC]

## Overview

These adapters make Protocol Buffer message types work with Pybind11 bindings.

To use the proto messages with pybind11:

1. Include the header file `pybind11_protobuf/native_proto_caster.h`
   in the .cc file with your bindings.
1. Call `pybind11_protobuf::ImportNativeProtoCasters();` in your `PYBIND11_MODULE` definition.
1. Ensure `"@com_google_protobuf//:protobuf_python"` is a python dependency.
   When using Bazel, a common strategy is to add a python library that "wraps"
   the extension along with any required python dependencies.

Any arguments or return values which are a protocol buffer (including the base
class, `proto2::Message`) will be automatically converted to python native
protocol buffers.


### Basic Example

```cpp
#include <pybind11/pybind11.h>

#include "path/to/my/my_message.proto.h"
#include "pybind11_protobuf/native_proto_caster.h"

// In real use, these two functions would probably be defined in a python-agnostic library.
MyMessage ReturnMyMessage() { ... }
void TakeMyMessage(const MyMessage& in) { ... }

PYBIND11_MODULE(my_module, m) {
  pybind11_protobuf::ImportNativeProtoCasters();

  m.def("return_my_message", &ReturnMyMessage);
  m.def("take_my_message", &TakeMyMessage, pybind11::arg("in"));
}
```


## Wrapped C++ vs Python Native Types

When passing protos between C++ and python, these bindings will convert the
protocol buffer into the native python type, which usually involves protocol
buffer serialization. The objects returned to python are the native python
protocol buffer objects, and so `isinstance()` and normal protocol buffer
operations and methods will behave as expected.

Enumerations are passed and returned as integers. You may use the enum values
from the native python proto module to set and check the enum values used
by a bound proto enum (see `tests/proto_enum_test.py` for an example).

Fundamentally sharing protocol buffer types between C++ and python runtimes
is unsafe because C++ assumes that it has full ownership of a protocol buffer
message, and may manipulate references in a way that undermines python
ownership semantics. Because of this sharing mutable references or pointers
between C++ and python is not allowed. However when using the python fast
proto implementation, the bindings may share an underlying protocol buffer
with C++ when passed by `const &` into a C++ function.

In cases where a protocol buffer is used as an in/out parameter in C++,
additional logic will be required in the wrapper.


### In / Out example

```cpp
#include <pybind11/pybind11.h>

#include "path/to/my/my_message.proto.h"
#include "pybind11_protobuf/native_proto_caster.h"

void MutateMessage(MyMessage* in_out) { ... }

PYBIND11_MODULE(my_module, m) {
  pybind11_protobuf::ImportNativeProtoCasters();

  m.def("mutate_message", [](MyMessage in) {
    MutateMessage(&in);
    return in;
  }),
  pybind11::arg("in"));
}
```


## Integration with the Old Way

Due to the nature of pybind11, extension modules built using `pybind11_protobuf/native_proto_caster.h`
cannot interoperate with the older bindings (those modules built using `pybind11_protobuf/proto_casters.h`)
as that may generate duplicate pybind11 specializations for a given protocol buffer
which would violate C++ ODR rules in an undetectable way. In order to workaround
that failure mode a nearly-transparent wrapper is also provided.

To use the wrappers:

1. Include the header file `pybind11_protobuf/wrapped_proto_caster.h`
   in the .cc file with your bindings.
1. Call `pybind11_protobuf::ImportWrappedProtoCasters();` in your `PYBIND11_MODULE` definition.
1. Wrap the C++ code with `pybind11_protobuf::WithWrappedProtos`.

### Wrapped Example

```cpp
#include <pybind11/pybind11.h>

#include "path/to/my/my_message.proto.h"
#include "pybind11_protobuf/wrapped_proto_caster.h"

// In real use, these 2 functions would probably be defined in a python-agnostic library.
MyMessage ReturnMyMessage() { ... }
void TakeMyMessage(const MyMessage& in) { ... }

PYBIND11_MODULE(my_module, m) {
  pybind11_protobuf::ImportWrappedProtoCasters();

  using pybind11_protobuf::WithWrappedProtos;

  m.def("return_my_message", WithWrappedProtos(&ReturnMyMessage));
  m.def("take_my_message", WithWrappedProtos(&TakeMyMessage), pybind11::arg("in"));
}
```
