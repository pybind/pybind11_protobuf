# Copyright (c) 2021 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.
"""Tests for protobuf casters."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from google3.testing.pybase import googletest
from google3.testing.pybase import parameterized
from pybind11_protobuf.tests import extension_module as m
from pybind11_protobuf.tests import extension_pb2


def get_py_message(value=5):
  """Returns a dynamic message that is wire-compatible with IntMessage."""
  msg = extension_pb2.BaseMessage()
  msg.Extensions[extension_pb2.int_message].value = value
  return msg


# NOTE: The isinstance checks guard against accessing the Extensions field
# when given a fast_cpp_proto and the binary is running in pure python mode,
# and indicates an access path which should be fixed.


class ProtoTest(parameterized.TestCase):

  @parameterized.named_parameters(('python', get_py_message),
                                  ('cpp', m.get_message))
  def test_full_name(self, get_message_function):
    a = get_message_function()
    self.assertEqual(str(a.DESCRIPTOR.full_name), 'pybind11.test.BaseMessage')

  def test_get_extension_py(self):
    a = get_py_message(value=8)
    self.assertEqual(8, a.Extensions[extension_pb2.int_message].value)

  def test_get_extension_cpp(self):
    a = m.get_message(value=8)
    if isinstance(a, extension_pb2.BaseMessage):
      self.assertEqual(8, a.Extensions[extension_pb2.int_message].value)
    else:
      with self.assertRaises(KeyError):
        a.Extensions[extension_pb2.int_message].value = 9

  def test_set_extension_cpp(self):
    a = m.get_base_message()
    if isinstance(a, extension_pb2.BaseMessage):
      a.Extensions[extension_pb2.int_message].value = 8
      self.assertEqual(8, a.Extensions[extension_pb2.int_message].value)
    else:
      with self.assertRaises(KeyError):
        a.Extensions[extension_pb2.int_message].value = 8

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


if __name__ == '__main__':
  googletest.main()
