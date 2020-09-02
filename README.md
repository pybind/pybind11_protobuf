# Pybind11 bindings for Google's Protocol Buffers

[TOC]

## Overview

These adapters make Protobuf types work with Pybind11 bindings. For more
information on using Pybind11, see
g3doc/third_party/pybind11/google3_utils/README.md.

To use the converters listed below, just include the header
in the .cc file with your bindings:

```
#include "pybind11_protobuf/proto_casters.h"
```

Any arguments or return values which are a protocol buffer (including the base
class, `proto2::Message`) will be automatically wrapped or converted.

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

## Features supported

- Singular, repeated, and map fields of all types.
- Serializing, parsing and merging messages.
- Descriptors, FieldDescriptors, EnumDescriptors and EnumValueDescriptors
  (partial support).
- Construction of a wrapped C proto from its name or equivalent native python
  proto, as well as keyword initialization.
- Python functions which can take protos as arguments:
  - `copy.deepcopy`
  - pickle (only to support copy.deepcopy; see go/nopickle).
  - `text_format` module methods `MessageToString`, `Parse` and `Merge`.
  - `compare.assertProto2Equal`.

## Features not yet implemented/ covered

- Oneof fields.
- Extensions.
- Slicing of repeated fields (b/145687883).

See proto.cc for a complete list of all bound and available methods.

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

