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
references or pointers between C++ and Python is not allowed.
However, when passing a Python protobuf object to
C++, and with `PYBIND11_PROTOBUF_ASSUME_FULL_ABI_COMPATIBILITY` defined
(see proto_cast_util.h),
the bindings will share the underlying C++ native protobuf object with C++ when
passed by `const &` or `const *`.

### Protobuf Extensions

When `use_fast_cpp_protos` is in use, and
protobuf extensions
are involved, a well-known pitfall is that extensions are silently moved
to the `proto2::UnknownFieldSet` when a message is deserialized in C++,
but the `cc_proto_library` for the extensions is not linked in. The root
cause is an asymmetry in the handling of Python protos vs C++ protos: when
a Python proto is deserialized, both the Python descriptor pool and the C++
descriptor pool are inspected, but when a C++ proto is deserialized, only
the C++ descriptor pool is inspected. Until this asymmetry is resolved, the
`cc_proto_library` for all extensions involved must be added to the `deps` of
the relevant `pybind_library` or `pybind_extension`, but this is sufficiently
unobvious to be a setup for regular accidents, potentially with critical
consequences.

To guard against the most common type of accident, native_proto_caster.h
includes a safety mechanism that raises "Proto Message has an Unknown Field"
in certain situations:

* When `use_fast_cpp_protos` is in use,
* a protobuf message is returned from C++ to Python,
* the message involves protobuf extensions (recursively),
* and the `proto2::UnknownFieldSet` for the message or any of its submessages
  is not empty.

`pybind11_protobuf::AllowUnknownFieldsFor` is an escape hatch for situations in
which

* unknown fields existed before the safety mechanism was
  introduced.
* unknown fields are needed in the future.

An example of a full error message (with lines breaks here for readability):

```
Proto Message of type pybind11.test.NestRepeated has an Unknown Field with
parent of type pybind11.test.BaseMessage: base_msgs.1003
(pybind11_protobuf/tests/extension_nest_repeated.proto,
pybind11_protobuf/tests/extension.proto).
Please add the required `cc_proto_library` `deps`.
Only if there is no alternative to suppressing this error, use
`pybind11_protobuf::AllowUnknownFieldsFor("pybind11.test.NestRepeated", "base_msgs");`
(Warning: suppressions may mask critical bugs.)
```

The current implementation is a compromise solution, trading off simplicity
of implementation, runtime performance, and precision. Generally, the runtime
overhead is expected to be very small, but fields flagged as unknown may not
necessarily be in extensions.
Alerting developers of new code to unknown fields is assumed to be generally
helpful, but the unknown fields detection is limited to messages with
extensions, to avoid the runtime overhead for the presumably much more common
case that no extensions are involved.

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
  },
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
