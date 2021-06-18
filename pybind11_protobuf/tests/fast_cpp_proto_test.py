# Copyright (c) 2019 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.
"""Tests for protobuf casters."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from google.protobuf import any_pb2
from google.protobuf import descriptor_pb2
from google.protobuf import descriptor_pool
from google.protobuf import message_factory
from google3.testing.pybase import googletest
from google3.testing.pybase import parameterized
from pybind11_protobuf.tests import compare
from pybind11_protobuf.tests import fast_cpp_proto_example as proto_example
from pybind11_protobuf.tests import test_pb2

POOL = descriptor_pool.DescriptorPool()
FACTORY = message_factory.MessageFactory(POOL)
POOL.Add(
    descriptor_pb2.FileDescriptorProto(
        name='pybind11_protobuf/tests',
        package='pybind11.test',
        message_type=[
            descriptor_pb2.DescriptorProto(
                name='DynamicMessage',
                field=[
                    descriptor_pb2.FieldDescriptorProto(
                        name='value', number=1, type=5)
                ]),
            descriptor_pb2.DescriptorProto(
                name='IntMessage',
                field=[
                    descriptor_pb2.FieldDescriptorProto(
                        name='value', number=1, type=5)
                ])
        ]))


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


def get_dynamic_message(value=5):
  """Returns a dynamic message that is wire-compatible with IntMessage."""
  prototype = FACTORY.CreatePrototype(
      POOL.FindMessageTypeByName('pybind11.test.DynamicMessage'))
  msg = prototype()
  msg.value = value
  return msg


def get_dynamic_int_message(value=5):
  """Returns a dynamic message named pybind11.test.IntMessage."""
  prototype = FACTORY.CreatePrototype(
      POOL.FindMessageTypeByName('pybind11.test.IntMessage'))
  msg = prototype()
  msg.value = value
  return msg


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


class ProtoTest(parameterized.TestCase, compare.ProtoAssertions):

  def test_type(self):
    # These are both seen as the concrete type.
    self.assertIn(
        str(type(proto_example.make_test_message())),
        [
            "<class 'pybind11_protobuf.tests.test_pb2.TestMessage'>",
            "<class 'TestMessage'>"  # native python protos.
        ])
    self.assertIn(
        str(type(proto_example.make_int_message())),
        [
            "<class 'pybind11_protobuf.tests.test_pb2.IntMessage'>",
            "<class 'IntMessage'>"  # native python protos.
        ])

  def test_full_name(self):
    self.assertEqual(
        str(test_pb2.IntMessage.DESCRIPTOR.full_name),
        'pybind11.test.IntMessage')
    self.assertEqual(
        str(proto_example.make_int_message().DESCRIPTOR.full_name),
        'pybind11.test.IntMessage')

  def test_str(self):
    a = test_pb2.IntMessage()
    a.value = 6
    b = proto_example.make_int_message()
    b.value = 6
    self.assertEqual(str(a), str(b))

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

  def test_get_dynamic_message_fails(self):
    with self.assertRaises(TypeError):
      proto_example.get_dynamic()

  # Do we make equality work across
  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('pybind11_wrapper', proto_example.make_int_message),
      ('int_message_unique_ptr', proto_example.get_int_message_unique_ptr),
      ('int_message_ptr_copy', proto_example.get_int_message_ptr_copy),
      ('int_message_ptr_take', proto_example.get_int_message_ptr_take),
      ('int_message_ref_copy', proto_example.get_int_message_ref_copy),
      ('message_unique_ptr', proto_example.get_message_unique_ptr))
  def test_equality(self, get_message_function):
    x = get_message_function()
    x.value = 5
    self.assertProtoEqual(test_pb2.IntMessage(value=5), x)

  @parameterized.named_parameters(
      ('native_proto', test_pb2.TestMessage),
      ('pybind11_wrapper', proto_example.make_test_message),
      ('string', lambda: 'not a proto'),  #
      ('python_dynamic', get_dynamic_message),
  )
  def test_pass_proto_wrong_type(self, get_message_function):
    message = get_message_function()
    with self.assertRaises(TypeError):
      proto_example.check_int_message(message, 5)

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

  def test_check_int_message_none(self):
    self.assertFalse(proto_example.check_int_message_const_ptr(None, 5))
    with self.assertRaises(TypeError):
      proto_example.check_int_message_const_ptr_notnone(None, 5)

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
      ('pybind11_wrapper', proto_example.make_int_message),
      ('dynamic_int_message', get_dynamic_int_message),
  )
  def test_check_message(self, get_message_function):
    message = get_message_function()
    message.value = 5
    self.assertTrue(proto_example.check_message(message, 5))
    self.assertTrue(proto_example.check_message_const_ptr(message, 5))
    if proto_example.PYBIND11_PROTOBUF_UNSAFE:
      self.assertTrue(proto_example.check_message_ptr(message, 5))
      self.assertTrue(proto_example.check_message_ref(message, 5))

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
    # these all make copies.
    self.assertTrue(proto_example.consume_int_message(message, 5))
    self.assertEqual(message.value, 5)
    self.assertTrue(proto_example.consume_message(message, 5))
    self.assertEqual(message.value, 5)
    self.assertTrue(proto_example.consume_int_message_const(message, 5))
    self.assertTrue(proto_example.consume_shared_int_message(message, 5))
    self.assertTrue(proto_example.consume_shared_int_message_const(message, 5))
    self.assertTrue(proto_example.consume_shared_message(message, 5))

  def test_consume_message_none(self):
    self.assertFalse(proto_example.consume_int_message(None, 5))
    self.assertFalse(proto_example.consume_message(None, 5))
    self.assertFalse(proto_example.consume_shared_int_message(None, 5))
    self.assertFalse(proto_example.consume_shared_message(None, 5))
    with self.assertRaises(TypeError):
      proto_example.consume_message_notnone(None, 5)
    with self.assertRaises(TypeError):
      proto_example.consume_int_message_notnone(None, 5)

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
    self.assertTrue(proto_example.check_message(message_1, 6))

  @parameterized.named_parameters(get_message())
  def test_get_message_fns(self, get_message_function):
    message = get_message_function()
    message.value = 5
    self.assertEqual(message.value, 5)
    self.assertTrue(proto_example.check_int_message(message, 5))
    self.assertTrue(proto_example.check_message(message, 5))

  def test_overload_fn(self):
    self.assertEqual(proto_example.fn_overload(test_pb2.IntMessage()), 2)
    self.assertEqual(proto_example.fn_overload(test_pb2.TestMessage()), 1)

  def test_enum(self):
    self.assertEqual(proto_example.adjust_enum(0), 1)
    self.assertEqual(
        proto_example.adjust_enum(test_pb2.TestMessage.ONE),
        test_pb2.TestMessage.TWO)
    with self.assertRaises(ValueError):
      proto_example.adjust_enum(7)

  @parameterized.named_parameters([('native_any_proto', any_pb2.Any,
                                    test_pb2.IntMessage),
                                   ('native_any_proto_pybind_msg', any_pb2.Any,
                                    proto_example.make_int_message)])
  def test_any_special_fields(self, get_any_function, get_message_function):
    message = get_message_function()
    message.value = 5
    any_proto = get_any_function()
    any_proto.Pack(message)
    self.assertEqual(any_proto.TypeName(), 'pybind11.test.IntMessage')
    self.assertEqual(any_proto.type_url,
                     'type.googleapis.com/pybind11.test.IntMessage')
    self.assertEqual(any_proto.value, b'\x08\x05')
    self.assertTrue(any_proto.Is(message.DESCRIPTOR))
    self.assertFalse(any_proto.Is(test_pb2.TestMessage().DESCRIPTOR))

    message.Clear()
    self.assertEqual(message.value, 0)
    self.assertTrue(any_proto.Unpack(message))
    self.assertEqual(message.value, 5)


if __name__ == '__main__':
  googletest.main()
