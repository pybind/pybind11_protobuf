# Copyright (c) 2019 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.
"""Tests for protobuf casters."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import unittest
import parameterized
from pybind11_protobuf.tests import fast_cpp_proto_example as proto_example
from pybind11_protobuf.tests import test_pb2
from google3.net.proto2.contrib.pyutil import compare


def get_fully_populated_test_message():
  """Returns a TestMessage with all fields set."""
  # This tests initializing a proto by keyword argument.
  return test_pb2.TestMessage(
      string_value='test',
      int_value=4,
      double_value=4.5,
      int_message=test_pb2.IntMessage(value=5),
      repeated_int_value=[6, 7],
      repeated_int_message=[test_pb2.IntMessage(value=8)],
      enum_value=test_pb2.TestMessage.TestEnum.ONE,
      repeated_enum_value=[test_pb2.TestMessage.TestEnum.TWO],
      string_int_map={'k': 5},
      int_message_map={1: test_pb2.IntMessage(value=6)})


def get_message_references():
  """Returns a parameter list of shared proto2 messages."""
  x = test_pb2.IntMessage()
  l = [('lambda', lambda: x)]
  if proto_example.PYBIND11_PROTOBUF_UNSAFE:
    l.extend([
        # ('int_shared_ptr', proto_example.get_int_message_shared_ptr),
        ('int_message_ref', proto_example.get_int_message_ref),
        ('int_message_raw_ptr', proto_example.get_int_message_raw_ptr),
        ('message_ref', proto_example.get_message_ref),
        ('message_raw_ptr', proto_example.get_message_raw_ptr)
    ])
  if proto_example.PYBIND11_PROTOBUF_UNSAFE and proto_example.REFERENCE_WRAPPER:
    l.extend([('int_message_ref_wrapper',
               proto_example.get_int_message_ref_wrapper),
              ('message_ref_wrapper', proto_example.get_message_ref_wrapper)])
  return l


def get_message():
  """Returns a parameter list of shared proto2 messages."""
  l = [
      ('native_proto', test_pb2.IntMessage),
      ('pybind11_wrapper', proto_example.make_int_message),
      ('int_message_const_ref', proto_example.get_int_message_const_ref),
      ('int_message_const_ptr', proto_example.get_int_message_const_raw_ptr),
      ('int_message_unique_ptr', proto_example.get_int_message_unique_ptr),
      ('int_message_ptr_copy', proto_example.get_int_message_ptr_copy),
      ('int_message_ptr_take', proto_example.get_int_message_ptr_take),
      ('int_message_ref_copy', proto_example.get_int_message_ref_copy),
      # functions that return proto2::Message in C++ convert to the concrete
      # type in python.
      ('message_const_ref', proto_example.get_message_const_ref),
      ('message_const_ptr', proto_example.get_message_const_raw_ptr),
      ('message_unique_ptr', proto_example.get_message_unique_ptr)
  ]
  if proto_example.REFERENCE_WRAPPER:
    l.extend([
        ('int_message_const_ref_wrapper',
         proto_example.get_int_message_const_ref_wrapper),
        ('int_message_ref_wrapper_copy',
         proto_example.get_int_message_ref_wrapper_copy),
    ])
  return l


class ProtoTest(compare.Proto2Assertions):

  def test_type(self):
    # These are both seen as the concrete type.
    self.assertEqual(
        str(type(proto_example.make_int_message())),
        "<class 'google3.third_party.pybind11_protobuf.tests.test_pb2.IntMessage'>"
    )
    self.assertEqual(
        str(type(proto_example.make_test_message())),
        "<class 'google3.third_party.pybind11_protobuf.tests.test_pb2.TestMessage'>"
    )

  def test_keep_alive_message(self):
    message = proto_example.make_test_message()
    field = message.int_message
    # message should be kept alive until field is also deleted.
    del message
    field.value = 5
    self.assertEqual(field.value, 5)

  def test_return_wrapped_message(self):
    message = proto_example.make_test_message()
    self.assertEqual(message.DESCRIPTOR.full_name, 'pybind11.test.TestMessage')
    self.assertEqual(message.__class__.DESCRIPTOR.full_name,
                     'pybind11.test.TestMessage')

  def test_get_message_none(self):
    if proto_example.PYBIND11_PROTOBUF_UNSAFE:
      self.assertIsNone(proto_example.get_int_message_raw_ptr_none())

  @parameterized.named_parameters(
      ('native_proto', test_pb2.TestMessage),
      ('pybind11_wrapper', proto_example.make_test_message),
      ('string', lambda: 'not a proto'))
  def test_pass_proto_wrong_type(self, get_message_function):
    message = get_message_function()
    self.assertRaises(TypeError, proto_example.check_int_message, message, 5)

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('pybind11_wrapper', proto_example.make_int_message))
  def test_check_int_message(self, get_message_function):
    message = get_message_function()
    message.value = 5
    self.assertTrue(proto_example.check_int_message(message, 5))

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('pybind11_wrapper', proto_example.make_int_message))
  def test_check_int_message_safe(self, get_message_function):
    message = get_message_function()
    message.value = 5
    self.assertTrue(proto_example.check_int_message_const_ptr(message, 5))
    self.assertTrue(proto_example.check_int_message_value(message, 5))
    self.assertTrue(proto_example.check_int_message_rvalue(message, 5))

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('pybind11_wrapper', proto_example.make_int_message))
  def test_check_int_message_unsafe(self, get_message_function):
    message = get_message_function()
    message.value = 5
    if proto_example.PYBIND11_PROTOBUF_UNSAFE:
      self.assertTrue(proto_example.check_int_message_ptr(message, 5))
      self.assertTrue(proto_example.check_int_message_ref(message, 5))

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('pybind11_wrapper', proto_example.make_int_message))
  def test_check_message(self, get_message_function):
    message = get_message_function()
    message.value = 5
    self.assertTrue(
        proto_example.check_message(message, message.DESCRIPTOR.full_name))
    self.assertTrue(
        proto_example.check_message_const_ptr(message,
                                              message.DESCRIPTOR.full_name))
    if proto_example.PYBIND11_PROTOBUF_UNSAFE:
      self.assertTrue(
          proto_example.check_message_ptr(message,
                                          message.DESCRIPTOR.full_name))
      self.assertTrue(
          proto_example.check_message_ref(message,
                                          message.DESCRIPTOR.full_name))

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('pybind11_wrapper', proto_example.make_int_message))
  def test_mutate_message(self, get_message_function):
    if proto_example.PYBIND11_PROTOBUF_UNSAFE:
      message = get_message_function()
      proto_example.mutate_int_message_ref(5, message)
      self.assertEqual(message.value, 5)
      proto_example.mutate_int_message_ptr(6, message)
      self.assertEqual(message.value, 6)

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('pybind11_wrapper', proto_example.make_int_message))
  def test_consume_int_message(self, get_message_function):
    message = get_message_function()
    message.value = 5
    proto_example.consume_int_message(message)  # makes a copy
    self.assertEqual(message.value, 5)
    proto_example.consume_message(message)  # makes another copy
    self.assertEqual(message.value, 5)

  @parameterized.named_parameters(get_message_references())
  def test_get_int_message_reference(self, get_message_function):
    message_1 = get_message_function()
    message_1.value = 5
    self.assertEqual(message_1.value, 5)
    message_2 = get_message_function()
    message_2.value = 6
    self.assertEqual(message_2.value, 6)
    # get_message_function always returns a reference to the same static
    # object, so message_1 and message_2 should always be equal.
    self.assertEqual(message_1.value, message_2.value)
    # test passing the message as a concrete type.
    self.assertTrue(proto_example.check_int_message(message_1, 6))
    # test passing the message as an abstract type.
    self.assertTrue(
        proto_example.check_message(message_1, message_1.DESCRIPTOR.full_name))

  @parameterized.named_parameters(get_message())
  def test_get_message_fns(self, get_message_function):
    message = get_message_function()
    message.value = 5
    self.assertEqual(message.value, 5)
    self.assertTrue(proto_example.check_int_message(message, 5))
    self.assertTrue(
        proto_example.check_message(message, message.DESCRIPTOR.full_name))

  def test_overload_fn(self):
    self.assertEqual(proto_example.fn_overload(test_pb2.IntMessage()), 2)
    self.assertEqual(proto_example.fn_overload(test_pb2.TestMessage()), 1)


if __name__ == '__main__':
  unittest.main()
