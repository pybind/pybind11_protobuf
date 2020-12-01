# Pybind11 bindings for Google's Protocol Buffers

[TOC]

## Before You Get Started

The bindings described below introduce a wrapped c++ protocol buffer mapping
for pybind11 which is currently in wide use but has the potential for
triggering undefined behavior from Python (b/173464573).

Since there is an existing python wrapper for C++ protocol buffers
(//net/proto2/python/public:use_fast_cpp_protos), and much of the work
overlaps, it makes sense to reuse the existing protocol implementation. That
type_caster, `fast_cpp_proto_casters.h`, is a work in progress found in this
directory.

New uses should consider using `fast_cpp_proto_casters.h` instead of the
bindings described below.  To get started, see the examples in
`test/fast_cpp_proto_*`.

## Overview

These adapters make Protocol Buffer message types work with Pybind11 bindings.
For more information on using Pybind11, see
g3doc/third_party/pybind11/google3_utils/README.md.

To use the proto messages with pybind11:

1. Include the header file `pybind11_protobuf/proto_casters.h`
   in the .cc file with your bindings.
1. Call `pybind11::google::ImportProtoModule();` in your `PYBIND11_MODULE` definition.
1. [Optional, not needed in most cases] Call `RegisterProtoMessageType`. See
   [below](#abstract-vs-concrete-message-bindings) for details.

Any arguments or return values which are a protocol buffer (including the base
class, `proto2::Message`) will be automatically wrapped or converted.

### Minimal Example

```cpp
#include <pybind11/pybind11.h>

#include "pybind11_protobuf/proto_casters.h"

PYBIND11_MODULE(my_module, m) {
  pybind11::google::ImportProtoModule();
}
```

### More Complete Example

```cpp
#include <pybind11/pybind11.h>

#include "path/to/my/my_message.proto.h"
#include "pybind11_protobuf/proto_casters.h"

// In real use, these 2 functions would probably be defined in a python-agnostic library.
MyMessage ReturnMyMessage() { ... }
void TakeMyMessage(const MyMessage& in) { ... }

PYBIND11_MODULE(my_module, m) {
  pybind11::google::ImportProtoModule();

  // RegisterProtoMessageType is not needed in this case, but shown as an example.
  pybind11::google::RegisterProtoMessageType<MyMessage>(m);

  m.def("return_my_message", &ReturnMyMessage);
  m.def("take_my_message", &TakeMyMessage, pybind11::arg("in"));
}
```

## Wrapped C++ vs Python Native Types

When returning a proto to python, this will apply a wrapper around the C++
structure rather constructing a native python proto. This wrapper attempts to
implement the native python proto API as closely as possible, but that API is
very large (dozens of methods), so coverage is not complete. However, all
the core machinery is there, and any part of the native API which is not covered
by the wrapper should be relatively easy to add, as long as it is supported by
the proto reflection API. Contact kenoslund@ if you want help with this.

If you really need a native python proto, serialize the wrapped proto and
deserialize it in the native type. However, it would be more computationally
efficient (and probably cleaner code) to add coverage for the part of the API
that you need and use the wrapped proto.

When passing protos from python into C++, this will attempt to remove the
wrapper if possible (ie, if the proto was returned from C via this converter).
When receiving a python native proto, this class will serialize it and
deserialize it into the corresponding C++ native proto, resulting in a copy.

Using a proto as an output (or input/output) argument is allowed, provided
the proto passed in is a wrapped C++ proto (not a python native proto).
When binding output arguments, use .noconvert() to enforce this.
See [here](https://pybind11.readthedocs.io/en/stable/advanced/functions.html#non-converting-arguments)
for details.

Enumerations are passed and returned as integers. You may use the enum values
from the native python proto module to set and check the enum values in a
wrapped proto (see [proto_test.py](http://google3../pybind11_protobuf/tests/proto_test.py)
for an example).

## Comparing Protos

The message objects returned from C++ to python are a different type than the
native Python message objects (even if the message type is the same) and
therefore will not compare equal with `==` even if the contents are indentical.

To check equality between protos in unit tests, use the [compare module]
(https://source.corp.google.com/piper///depot/google3/net/proto2/contrib/pyutil/compare.py).
This also has the advantage of providing more useful error messages when the
protos do not compare equally.

Outside of unit tests, compare protos field-by-field or serialize/deserialize
to convert both into native or both into wrapped-C++ protos.

## Abstract vs Concrete Message Bindings

The proto module includes bindings for the abstract `proto2::Message` base
class, and this is all that's necessary in most cases. However, some features
require registering the concrete message type by adding this to your
`PYBIND11_MODULE` definition:

```
pybind11::google::RegisterProtoMessageType<MyMessageType>(my_module);
```

That line does the following:

- Adds a constructor for `MyMessageType` to your module (`my_module.MyMessageType()`
  in this case). The constructor accepts keyword arguments to initialize fields,
  like native Python constructors.
- Adds a constructor to the `__class__` attribute. This allows constructing a
  new instance of the same message type with `message.__class__(**kwargs)`,
  which is required in a few places (for example, by `assertProto2Equal`).
- `DESCRIPTOR` is a static property of `my_module.MyMessageType()` rather than
  instance property.
- The fields are directly registered as instance properties rather than
  accessed through `__getattr__` and `__setattr__`. This saves a call to
  `Descriptor::FindFieldByName` each time the field is accessed and may be
  slightly more efficient.
- `type(my_message_instance)` will give you a concrete type (python sees all
  unregistered messages as the base class type, specifically
  `google3.third_party.pybind11_protobuf.proto.ProtoMessage`).

## Importing the proto module

For the proto casters to work, bindings for the generic protobuffer types (eg,
`proto2::Message`) must be registered with pybind11. These can be imported from
python in the usual way (`from pybind11_protobuf import proto`)
or by calling `pybind11::google::ImportProtoModule()` from the `PYBIND11_MODULE`
definition of modules which use the proto casters. As usual, the import can be
done any number of times in the same program; anything after the first will just
get the already-imported module. Importing the bindings both ways in either
order also works as expected (but only python 3; this will probably cause errors
in python 2).

When `ImportProtoModule` is called, the bindings will be added from linked
symbols, so that the `proto` module does *not* have to exist on disk and .so
files for modules which use the proto casters are fully self-contained.
However, it is *possible* to have a corresponding module on disk, which can be
imported on its own, as is the case with the `pybind11_protobuf/proto.cc` module.
In that case, `PYBIND11_PROTOBUF_MODULE_PATH` must match the path to this module.

The status casters use the same import mechanism; if modifying the following
functions, make the same changes in the corresponding status functions:
- ImportProtoModule
- IsProtoModuleImported
- CheckProtoModuleImported

## Protocol Buffer Holder

Internally pybind11 uses a smart pointer to hold wrapped C++ types (see [here]
(https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html) for details).
All instances of a given type must use the same the holder type; using the wrong
holder type will cause a segfault without a useful error message.

Protocol buffers all use `std::shared_ptr` as their holder. Inside of google3,
we have patched pybind11 so that a `std::unique_ptr` to a protocol buffer will
automatically be converted to a `std::shared_ptr`. However, this does not
currently work with the open source release of pybind11, so this functionality
is stripped out by copybara. Therefore, open sourced code *must* manually convert
a `std::unique_ptr` to a proto to a `std::shared_ptr` before returning it to python.
Enabling the automatic conversion in the core pybind11 is a WIP but will take
more time. Returning protocol buffers by value (ie, without a smart pointer)
and inside a `std::shared_ptr` *will* work automatically with the open source
release of pybind11.

## Use Outside of Google3

When exporting code to be built outside of google3, the module path used
for the base class proto bindings can be changed by defining
`PYBIND11_PROTOBUF_MODULE_PATH`. Since bindings for the generic protobuffer
types are loaded from linked symbols, the module path defined can be any
arbitrary value (as long as it does not conflict with other module names) and
does not have correspond to an actual .so file on disk. An example of changing
this path is in the tests.

## Features supported

- Singular, repeated, and map fields of all types.
- Serializing, parsing and merging messages.
- Descriptors, FieldDescriptors, EnumDescriptors and EnumValueDescriptors
  (partial support).
- Construction of a wrapped C proto from its name or equivalent native python
  proto, as well as keyword initialization.
- Python functions which can take protos as arguments:
  - `copy.deepcopy`
  - pickle (needed for e.g. beam and useful for debugging; but see go/nopickle).
  - `text_format` module methods `MessageToString`, `Parse` and `Merge`.
  - `compare.assertProto2Equal`.
- Most commonly used Message methods, including:
  - SerializeToString
  - ParseFromString
  - MergeFromString
  - ByteSize
  - Clear
  - CopyFrom
  - MergeFrom
  - FindInitializationErrors
  - ListFields
  - HasField (including support for oneof fields).
  - ClearField (including support for oneof fields).
  - WhichOneOf

## Features not yet implemented/ covered

- Extensions.
- Slicing of repeated fields (b/145687883).

See proto_utils.cc for a complete list of all available methods.

## FAQ/Troubleshooting

1. Can C++ code reference (and optionally modify) a proto owned by python
   without copying it?

   Yes, as long as it's a wrapped C++ proto, not a native python proto.

2. How do I construct a wrapped C++ proto from python?

   Use `RegisterProtoMessageType`- that will add a constructor to your module.
   See above for details.

3. I get `TypeError: MyMessageType: No constructor defined!`

   Call `RegisterProtoMessageType<MyMessageType>` in your `PYBIND11_MODULE`
   definition.

## Reporting bugs

The bindings are designed to exactly match the [native python proto API]
(https://developers.google.com/protocol-buffers/docs/reference/python-generated).
However, there are still some differences. If you discover a difference that
impacts your use case, please [check if there is a bug for it]
(https://b.corp.google.com/issues?q=componentid:779382%20status:open)
and [file a bug if there is none]
(https://b.corp.google.com/issues/new?component=779382&template=1371463).

