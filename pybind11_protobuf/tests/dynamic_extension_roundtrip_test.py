# Copyright (c) 2024 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.
"""Regression test for use-after-free in FindFileContainingExtension.

When a dynamic Python proto with extensions is passed through pybind11_protobuf,
the C++ bridge creates a DescriptorPool backed by the Python pool. During
proto parsing on the C++ side, extension lookup calls back into the Python
pool via FindExtensionByNumber. If the intermediate FieldDescriptor wrapper
returned by FindExtensionByNumber is not kept alive while its `.file` attribute
is accessed and serialized, the UPB runtime may free the wrapper before
DescriptorPoolDatabase::CopyToFileDescriptorProto finishes, leading to a
heap-use-after-free.

This test creates a fully dynamic proto schema (base message + extension in a
separate file), serializes a message with the extension set, then roundtrips
it through a C++ pybind11 function that accepts `const proto2::Message&`.
Without the fix in proto_cast_util.cc, this crashes under AddressSanitizer.
"""

import gc

from absl.testing import absltest
from google.protobuf import descriptor_pb2
from google.protobuf import descriptor_pool
from google.protobuf import message_factory

from pybind11_protobuf.tests import dynamic_message_module as m


def _make_base_file():
  """Create a FileDescriptorProto with an extendable message."""
  return descriptor_pb2.FileDescriptorProto(
      name='_dynamic_ext_test_base.proto',
      package='pybind11.dynamic_ext_test',
      syntax='proto2',
      message_type=[
          descriptor_pb2.DescriptorProto(
              name='Extendable',
              field=[
                  descriptor_pb2.FieldDescriptorProto(
                      name='id',
                      number=1,
                      type=descriptor_pb2.FieldDescriptorProto.TYPE_INT32,
                      label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL,
                  ),
              ],
              extension_range=[
                  descriptor_pb2.DescriptorProto.ExtensionRange(
                      start=100, end=10000
                  ),
              ],
          ),
      ],
  )


def _make_ext_file():
  """Create a FileDescriptorProto defining an extension in a separate file."""
  return descriptor_pb2.FileDescriptorProto(
      name='_dynamic_ext_test_ext.proto',
      package='pybind11.dynamic_ext_test',
      syntax='proto2',
      dependency=['_dynamic_ext_test_base.proto'],
      message_type=[
          descriptor_pb2.DescriptorProto(
              name='Annotation',
              field=[
                  descriptor_pb2.FieldDescriptorProto(
                      name='value',
                      number=1,
                      type=descriptor_pb2.FieldDescriptorProto.TYPE_STRING,
                      label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL,
                  ),
              ],
          ),
      ],
      extension=[
          descriptor_pb2.FieldDescriptorProto(
              name='annotation_ext',
              number=200,
              type=descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE,
              type_name='.pybind11.dynamic_ext_test.Annotation',
              extendee='.pybind11.dynamic_ext_test.Extendable',
              label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL,
          ),
      ],
  )


class DynamicExtensionRoundtripTest(absltest.TestCase):
  """Regression test: dynamic proto extensions via pybind11 roundtrip."""

  def test_roundtrip_dynamic_extension(self):
    """A dynamic proto with extensions must survive a pybind11 roundtrip."""
    base_file = _make_base_file()
    ext_file = _make_ext_file()

    # Create a pool with both the base and extension files.
    pool = descriptor_pool.DescriptorPool()
    pool.Add(base_file)
    pool.Add(ext_file)

    classes = message_factory.GetMessageClassesForFiles(
        [base_file.name, ext_file.name], pool
    )

    # Build a message with the extension set.
    extendable_cls = classes['pybind11.dynamic_ext_test.Extendable']
    ext_desc = pool.FindExtensionByName(
        'pybind11.dynamic_ext_test.annotation_ext'
    )
    msg = extendable_cls()
    msg.id = 42
    msg.Extensions[ext_desc].value = 'hello'

    # Drop reference to descriptor and force GC to ensure the wrapper is
    # collected if not held elsewhere, triggering the UAF when C++ tries
    # to look it up again.
    del ext_desc
    gc.collect()

    # Roundtrip through C++ via `const proto2::Message&`.
    # This triggers AllocateCProtoFromPythonSymbolDatabase which wraps the
    # Python pool in a C++ DescriptorPool, and ParsePartialFromString which
    # triggers FindFileContainingExtension to look up the extension.
    result = m.roundtrip(msg)

    # Verify the message survived the roundtrip.
    # The result is a Python proto from the same dynamic pool.
    self.assertEqual(result.id, 42)

    # The extension data should be preserved (either as a known extension
    # or as unknown fields that re-serialize identically).
    self.assertEqual(
        result.SerializeToString(), msg.SerializeToString()
    )


if __name__ == '__main__':
  absltest.main()
