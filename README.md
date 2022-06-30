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


## C++ Native vs Python Native Types

When passing protos between C++ and Python, the `native_proto_caster.h`
bindings will convert protobuf objects to the native type on either side.

While C++ has only one native type, Python has two native types
(https://rules-proto-grpc.com/en/latest/lang/python.html):

* `--define=use_fast_cpp_protos=false` (aka `use_pure_python_protos`)
* `--define=use_fast_cpp_protos=true`

With `use_pure_python_protos`, protobuf objects passed between C++ and Python
are always serialized/deserialized between the native C++ type and the pure
Python native type. This is very safe but also slow.

With `use_fast_cpp_protos`, the native Python type is internally backed by
the native C++ type, which unlocks various performance benefits, even when
only used from Python. When passing protobuf objects between Python and C++,
in certain situations the serialization/deserialization overhead can be
avoided, but not universally. Fundamentally, sharing C++ native protobuf
objects between C++ and Python is unsafe because C++ assumes that it has
exclusive ownership and may manipulate references in a way that undermines
Python's much safer ownership semantics. Because of this, sharing mutable
references or pointers between C++ and Python is not allowed. However, when
passing a Python protobuf object to C++, the bindings may share the underlying
C++ native protobuf object with C++ when passed by `const &` or `const *`.

### Protobuf Extensions

With `use_fast_cpp_protos`, any `py_proto_library` becomes implicitly dependent
on the corresponding `cc_proto_library`, but this is currently not handled
automatically, nor clearly diagnosed or formally enforced. In the absence of
protobuf extensions
usually there is no problem: Python code tends to depend organically on
a given `py_proto_library`, and pybind11-wrapped C++ code tends to depend
organically on the corresponding `cc_proto_library`. However, when extensions
are involved, a well-known pitfall is to accidentally omit the corresponding
`cc_proto_library`. Currently this needs to be kept in mind as a pitfall
(sorry), but the usual best-practice unit testing is likely to catch such
situations. Once discovered, the fix is easy: add the `cc_proto_library`
to the `deps` of the relevant `pybind_library` or `pybind_extension`.

### Enumerations

Enumerations are passed and returned as integers. You may use the enum values
from the native python proto module to set and check the enum values used
by a bound proto enum (see `tests/proto_enum_test.py` for an example).

### In / Out Parameters

In cases where a protocol buffer is used as an in/out parameter in C++,
additional logic will be required in the wrapper. For example:

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


### `pybind11_protobuf/wrapped_proto_caster.h`

TL;DR: Ignore `wrapped_proto_caster.h` if you can, this header was added as
a migration aid before the removal of `proto_casters.h`.

Historic background: Due to the nature of pybind11, extension modules
built using `native_proto_caster.h` could not interoperate with the
older `proto_casters.h` bindings, as that would have led to C++ ODR
violations. `wrapped_proto_caster.h` is a nearly-transparent wrapper for
`native_proto_caster.h` to work around the ODR issue. With the migration to
`native_proto_caster.h` now completed, `wrapped_proto_caster.h` is obsolete
and will be removed in the future.
