# Copyright (c) 2021 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.
"""Tests for protobuf casters."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from absl.testing import absltest
from absl.testing import parameterized

from google.protobuf.internal import api_implementation
from pybind11_protobuf.tests import extension_in_other_file_in_deps_pb2
from pybind11_protobuf.tests import extension_in_other_file_pb2
from pybind11_protobuf.tests import extension_module as m
from pybind11_protobuf.tests import extension_pb2


def get_py_message(value=5,
                   in_other_file_in_deps_value=None,
                   in_other_file_value=None):
  """Returns a dynamic message that is wire-compatible with IntMessage."""
  msg = extension_pb2.BaseMessage()
  if in_other_file_in_deps_value is not None:
    msg.Extensions[
        extension_in_other_file_in_deps_pb2.MessageInOtherFileInDeps.
        message_in_other_file_in_deps_extension].value = in_other_file_in_deps_value
  elif in_other_file_value is not None:
    msg.Extensions[extension_in_other_file_pb2.MessageInOtherFile
                   .message_in_other_file_extension].value = in_other_file_value
  else:
    msg.Extensions[extension_pb2.int_message].value = value
  return msg


class ExtensionTest(parameterized.TestCase):

  @parameterized.named_parameters(('python', get_py_message),
                                  ('cpp', m.get_message))
  def test_full_name(self, get_message_function):
    a = get_message_function()
    self.assertEqual(str(a.DESCRIPTOR.full_name), 'pybind11.test.BaseMessage')

  @parameterized.named_parameters(('python', get_py_message),
                                  ('cpp', m.get_message))
  def test_get_extension_py(self, get_message_function):
    a = get_message_function(value=8)
    self.assertEqual(8, a.Extensions[extension_pb2.int_message].value)

  @parameterized.named_parameters(('python', get_py_message),
                                  ('cpp', m.get_message))
  def test_set_extension_cpp(self, get_message_function):
    a = get_message_function()
    a.Extensions[extension_pb2.int_message].value = 8
    self.assertEqual(8, a.Extensions[extension_pb2.int_message].value)

  @parameterized.named_parameters(('python', get_py_message),
                                  ('cpp', m.get_message))
  def test_extension_fn(self, get_message_function):
    a = get_message_function(value=7)
    b = m.get_extension(a)
    self.assertEqual(7, b.value)

  @parameterized.named_parameters(('python', get_py_message),
                                  ('cpp', m.get_message))
  def test_extension_fn_roundtrip(self, get_message_function):
    a = get_message_function(value=7)
    b = m.get_extension(m.roundtrip(a))
    self.assertEqual(7, b.value)

  @parameterized.parameters('roundtrip', 'roundtrip_with_serialize_deserialize')
  def test_extension_in_same_file(self, roundtrip_fn):
    a = get_py_message(94)
    b = getattr(m, roundtrip_fn)(a)
    self.assertEqual(94, b.Extensions[extension_pb2.int_message].value)

  @parameterized.parameters('roundtrip', 'roundtrip_with_serialize_deserialize')
  def test_extension_in_other_file_in_deps(self, roundtrip_fn):
    a = get_py_message(in_other_file_in_deps_value=38)
    b = getattr(m, roundtrip_fn)(a)
    self.assertEqual(
        38, b.Extensions[
            extension_in_other_file_in_deps_pb2.MessageInOtherFileInDeps
            .message_in_other_file_in_deps_extension].value)

  def test_extension_in_other_file_roundtrip(self):
    a = get_py_message(in_other_file_value=57)
    b = m.roundtrip(a)
    self.assertEqual(
        57, b.Extensions[extension_in_other_file_pb2.MessageInOtherFile
                         .message_in_other_file_extension].value)

  def test_extension_in_other_file_roundtrip_with_serialize_deserialize(self):
    a = get_py_message(in_other_file_value=63)
    b = m.roundtrip_with_serialize_deserialize(a)
    b_value = b.Extensions[extension_in_other_file_pb2.MessageInOtherFile
                           .message_in_other_file_extension].value
    if api_implementation.Type() == 'cpp':
      self.assertEqual(0, b_value)  # SILENT FAILURE (similar to b/74017912)
    else:
      self.assertEqual(63, b_value)


if __name__ == '__main__':
  absltest.main()
