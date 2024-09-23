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

from google3.net.proto2.proto import descriptor_pb2
from google3.net.proto2.python.public import descriptor_pool
from google3.net.proto2.python.public import message_factory
from pybind11_protobuf.tests import compare
from pybind11_protobuf.tests import dynamic_message_module as m
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


def get_py_dynamic_message(value=5):
  """Returns a dynamic message that is wire-compatible with IntMessage."""
  prototype = FACTORY.CreatePrototype(
      POOL.FindMessageTypeByName('pybind11.test.DynamicMessage'))
  msg = prototype(value=value)
  return msg


def get_py_dynamic_int_message(value=5):
  """Returns a dynamic message named pybind11.test.IntMessage."""
  prototype = FACTORY.CreatePrototype(
      POOL.FindMessageTypeByName('pybind11.test.IntMessage'))
  msg = prototype(value=value)
  return msg


def get_cpp_dynamic_int_message(value=5):
  """Returns a dynamic message named pybind11.test.IntMessage."""
  return m.dynamic_message_ptr('pybind11.test.IntMessage', value)


def get_cpp_dynamic_message(value=5):
  """Returns a dynamic message that is wire-compatible with IntMessage."""
  return m.dynamic_message_ptr('pybind11.test.DynamicMessage', value)


def get_cpp_dynamic_message_unique_ptr(value=5):
  """Returns a dynamic message that is wire-compatible with IntMessage."""
  return m.dynamic_message_unique_ptr('pybind11.test.DynamicMessage', value)


def get_cpp_dynamic_message_shared_ptr(value=5):
  """Returns a dynamic message that is wire-compatible with IntMessage."""
  return m.dynamic_message_shared_ptr('pybind11.test.DynamicMessage', value)


class DynamicMessageTest(parameterized.TestCase, compare.ProtoAssertions):

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('py_dynamic_int', get_py_dynamic_int_message),
      ('cpp_dynamic_int', get_cpp_dynamic_int_message))
  def test_full_name(self, get_message_function):
    message = get_message_function()
    self.assertEqual(
        str(message.DESCRIPTOR.full_name), 'pybind11.test.IntMessage')

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('py_dynamic_int', get_py_dynamic_int_message),
      ('cpp_dynamic_int', get_cpp_dynamic_int_message),
      ('py_dynamic', get_py_dynamic_message),
  )
  def test_check_message(self, get_message_function):
    message = get_message_function(value=5)
    self.assertTrue(m.check_message(message, 5))
    self.assertTrue(m.check_message_const_ptr(message, 5))

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('py_dynamic_int', get_py_dynamic_int_message),
      ('cpp_dynamic_int', get_cpp_dynamic_int_message),
  )
  def test_roundtrip(self, get_message_function):
    a = get_message_function(value=6)
    b = m.roundtrip(a)
    self.assertTrue(m.check_message(b, 6))

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('py_dynamic_int', get_py_dynamic_int_message),
      ('cpp_dynamic_int', get_cpp_dynamic_int_message),
  )
  def test_parse_as(self, get_message_function):
    a = get_message_function(value=6)
    b = m.parse_as('value: 333', a)
    self.assertTrue(m.check_message(b, 333), b)

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('py_dynamic_int', get_py_dynamic_int_message),
      ('cpp_dynamic_int', get_cpp_dynamic_int_message),
      ('py_dynamic', get_py_dynamic_message),
  )
  def test_print(self, get_message_function):
    a = get_message_function(value=6)
    b = m.print(a)
    self.assertEqual(b.strip(), 'value: 6', b)

  @parameterized.named_parameters(
      ('native_proto', test_pb2.IntMessage),
      ('py_dynamic_int', get_py_dynamic_int_message),
      ('cpp_dynamic_int', get_cpp_dynamic_int_message),
      ('py_dynamic', get_py_dynamic_message),
  )
  def test_print_descriptor(self, get_message_function):
    a = get_message_function(value=6)
    b = m.print_descriptor(a)
    self.assertNotEqual(-1, b.find('value = 1'), b)


if __name__ == '__main__':
  absltest.main()
