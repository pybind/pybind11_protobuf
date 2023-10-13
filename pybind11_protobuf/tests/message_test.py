# Copyright (c) 2021 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.
"""Tests for basic proto operations on pybind11 type_cast<> protobufs."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import copy
import pickle
import re

from absl.testing import absltest
from absl.testing import parameterized

from google.protobuf import any_pb2
from pybind11_protobuf.tests import compare
from pybind11_protobuf.tests import message_module as m
from pybind11_protobuf.tests import test_pb2
from google.protobuf import text_format


def get_py_message():
  """Returns a native TestMessage with all fields set."""
  return text_format.Parse(m.TEXT_FORMAT_MESSAGE, test_pb2.TestMessage())


def get_cpp_message():
  """Returns a wrapped TestMessage with all fields set."""
  return m.make_test_message(m.TEXT_FORMAT_MESSAGE)


def remove_ws(text):
  return re.sub(r'\s+', '', str(text))


class MessageTest(parameterized.TestCase, compare.ProtoAssertions):

  @parameterized.named_parameters(
      ('native_proto', test_pb2.TestMessage),
      ('cast_proto', m.make_test_message),
  )
  def test_isinstance(self, factory):
    self.assertIsInstance(factory(), test_pb2.TestMessage)

  @parameterized.named_parameters(
      ('native_proto', test_pb2.TestMessage),
      ('cast_proto', m.make_test_message),
  )
  def test_access_singluar_fields(self, factory):
    message = factory()
    message.string_value = 'test'
    message.int_value = 5
    message.double_value = 5.5
    message.int_message.value = 6
    self.assertEqual(message.string_value, 'test')
    self.assertEqual(message.int_value, 5)
    self.assertEqual(message.double_value, 5.5)
    self.assertEqual(message.int_message.value, 6)

  @parameterized.named_parameters(
      ('native_proto', test_pb2.TestMessage),
      ('cast_proto', m.make_test_message),
  )
  def test_access_repeated_int_value(self, factory):
    message = factory()
    message.repeated_int_value.append(6)
    message.repeated_int_value.append(7)

    self.assertLen(message.repeated_int_value, 2)
    self.assertEqual(message.repeated_int_value[0], 6)
    self.assertEqual(message.repeated_int_value[1], 7)
    for value, expected in zip(message.repeated_int_value, [6, 7]):
      self.assertEqual(value, expected)

    self.assertEqual(str(message.repeated_int_value), '[6, 7]')

    message.repeated_int_value[0] = 8
    self.assertSequenceEqual(message.repeated_int_value, [8, 7])

    message.repeated_int_value.insert(1, 2)
    self.assertSequenceEqual(message.repeated_int_value, [8, 2, 7])

    del message.repeated_int_value[1]
    self.assertSequenceEqual(message.repeated_int_value, [8, 7])

    message.repeated_int_value.extend([6, 5])
    self.assertSequenceEqual(message.repeated_int_value, [8, 7, 6, 5])

    self.assertEqual(message.repeated_int_value[:], [8, 7, 6, 5])
    self.assertEqual(message.repeated_int_value[:2], [8, 7])
    self.assertEqual(message.repeated_int_value[1:3], [7, 6])
    self.assertEqual(message.repeated_int_value[2:], [6, 5])
    self.assertEqual(message.repeated_int_value[::2], [8, 6])  # step = 2

    message.repeated_int_value[1] = 5
    self.assertSequenceEqual(message.repeated_int_value, [8, 5, 6, 5])

    message.repeated_int_value[1:3] = [0, 1]
    self.assertSequenceEqual(message.repeated_int_value, [8, 0, 1, 5])

    del message.repeated_int_value[1:3]
    self.assertSequenceEqual(message.repeated_int_value, [8, 5])

    del message.repeated_int_value[:]
    self.assertEmpty(message.repeated_int_value)

    with self.assertRaises(TypeError):
      message.repeated_int_value.append('invalid value')
    with self.assertRaises(AttributeError):
      message.repeated_int_value = [1]
    with self.assertRaises(IndexError):
      print(message.repeated_int_value[1000])

  def test_access_int_message_field(self):
    message = m.make_test_message()
    with self.assertRaises(AttributeError):
      message.int_message = test_pb2.IntMessage()
    with self.assertRaises(AttributeError):
      message.int_message = m.make_int_message(4)

  def test_construct_nested_message(self):
    n = m.make_nested_message(4)
    self.assertEqual(4, n.value)

  def test_access_nested_message_field(self):
    message = m.make_test_message()
    message.nested.value = 4
    self.assertEqual(4, message.nested.value)
    with self.assertRaises(AttributeError):
      message.nested = test_pb2.IntMessage.Nested()
    with self.assertRaises(AttributeError):
      message.nested = m.make_nested_message(4)

  def test_access_repeated_int_message(self):
    message = m.make_test_message()
    sub_msg = m.make_int_message()
    sub_msg.value = 6
    message.repeated_int_message.append(sub_msg)
    # Append/Extend/Set should work from native or wrapped messages.
    sub_msg = test_pb2.IntMessage()
    sub_msg.value = 7
    message.repeated_int_message.append(sub_msg)

    def check_values(messages, expected_values):
      self.assertLen(messages, len(expected_values))
      for msg, expected in zip(messages, expected_values):
        self.assertEqual(msg.value, expected)

    self.assertLen(message.repeated_int_message, 2)
    self.assertEqual(message.repeated_int_message[0].value, 6)
    self.assertEqual(message.repeated_int_message[1].value, 7)
    check_values(message.repeated_int_message, [6, 7])

    self.assertEqual(
        remove_ws(str(message.repeated_int_message)), '[value:6,value:7]')

    message.repeated_int_message[0].value = 8
    check_values(message.repeated_int_message, [8, 7])

    sub_msg.value = 2
    message.repeated_int_message.insert(1, sub_msg)
    check_values(message.repeated_int_message, [8, 2, 7])

    message.repeated_int_message.extend([sub_msg, sub_msg])
    check_values(message.repeated_int_message, [8, 2, 7, 2, 2])

    new_msg = message.repeated_int_message.add()
    self.assertEqual(new_msg.value, 0)
    check_values(message.repeated_int_message, [8, 2, 7, 2, 2, 0])

    new_msg = message.repeated_int_message.add(value=3)
    self.assertEqual(new_msg.value, 3)
    check_values(message.repeated_int_message, [8, 2, 7, 2, 2, 0, 3])

    with self.assertRaises(ValueError):
      message.repeated_int_message.add(invalid_field=3)
    check_values(message.repeated_int_message, [8, 2, 7, 2, 2, 0, 3])

    check_values(message.repeated_int_message[:], [8, 2, 7, 2, 2, 0, 3])
    check_values(message.repeated_int_message[3:], [2, 2, 0, 3])
    check_values(message.repeated_int_message[2:4], [7, 2])
    check_values(message.repeated_int_message[:3], [8, 2, 7])
    check_values(message.repeated_int_message[::2], [8, 7, 2, 3])  # step = 2

    del message.repeated_int_message[1]
    check_values(message.repeated_int_message, [8, 7, 2, 2, 0, 3])

    del message.repeated_int_message[2:4]
    check_values(message.repeated_int_message, [8, 7, 0, 3])

    del message.repeated_int_message[:]
    self.assertEmpty(message.repeated_int_message)

    with self.assertRaises(IndexError):
      print(message.repeated_int_message[1000])

    with self.assertRaises(TypeError):
      message.repeated_int_message.append('invalid value')
    with self.assertRaises(TypeError):
      message.repeated_int_message.append(test_pb2.TestMessage())
    with self.assertRaises(AttributeError):
      message.repeated_int_message = [test_pb2.IntMessage()]

    message.repeated_int_message.add()
    with self.assertRaises((TypeError, AttributeError)):
      message.repeated_int_message[0] = test_pb2.IntMessage()
    message.repeated_int_message[0].CopyFrom(test_pb2.IntMessage())

  def test_access_map_string_int(self):
    message = m.make_test_message()

    message.string_int_map['k1'] = 5
    message.string_int_map['k2'] = 6
    self.assertLen(message.string_int_map, 2)
    self.assertEqual(message.string_int_map['k1'], 5)
    self.assertEqual(message.string_int_map['k2'], 6)
    self.assertIn('k1', message.string_int_map)
    self.assertIn('k2', message.string_int_map)
    self.assertNotIn('k3', message.string_int_map)

    container = message.string_int_map
    for key, expected in zip(sorted(container), ['k1', 'k2']):
      self.assertEqual(key, expected)

    for key, expected in zip(sorted(container.keys()), ['k1', 'k2']):
      self.assertEqual(key, expected)

    for value, expected in zip(sorted(container.values()), [5, 6]):
      self.assertEqual(value, expected)

    for item, expected in zip(
        sorted(container.items()), [('k1', 5), ('k2', 6)]):
      self.assertEqual(item, expected)

    native = test_pb2.TestMessage()
    native.string_int_map['k1'] = 5
    native.string_int_map['k2'] = 6
    self.assertDictEqual(
        dict(message.string_int_map), dict(native.string_int_map))

    message.string_int_map.update(k3=7)
    self.assertEqual(message.string_int_map['k3'], 7)
    message.string_int_map.update({'k4': 8})
    self.assertEqual(message.string_int_map['k4'], 8)

    message.string_int_map.clear()
    self.assertEmpty(message.string_int_map)

    with self.assertRaises(TypeError):
      message.string_int_map[5] = 5  # invalid key.
    with self.assertRaises(TypeError):
      message.string_int_map['k'] = 'foo'  # invalid value.
    with self.assertRaises(AttributeError):
      message.string_int_map = {'k': 5}

  def test_access_map_int_message(self):
    message = m.make_test_message()

    message.int_message_map[5].value = 2
    message.int_message_map[6].value = 3
    self.assertLen(message.int_message_map, 2)
    self.assertEqual(message.int_message_map[5].value, 2)
    self.assertEqual(message.int_message_map[6].value, 3)
    self.assertIn(5, message.int_message_map)
    self.assertIn(6, message.int_message_map)
    self.assertNotIn(7, message.int_message_map)

    container = message.int_message_map
    for key, expected in zip(sorted(container), [5, 6]):
      self.assertEqual(key, expected)

    for key, expected in zip(sorted(container.keys()), [5, 6]):
      self.assertEqual(key, expected)

    msg_values = [msg.value for msg in container.values()]
    for value, expected in zip(sorted(msg_values), [2, 3]):
      self.assertEqual(value, expected)

    msg_items = [(item[0], item[1].value) for item in container.items()]
    for item, expected in zip(sorted(msg_items), [(5, 2), (6, 3)]):
      self.assertEqual(item, expected)

    self.assertIn(
        remove_ws(str(message.int_message_map)),
        ['{5:value:2,6:value:3}', '{6:value:3,5:value:2}'])

    with self.assertRaises(ValueError):
      message.int_message_map.update({7: test_pb2.IntMessage(value=8)})
    with self.assertRaises(ValueError):
      message.int_message_map.update({7: m.make_int_message(value=8)})

    message.int_message_map.clear()
    self.assertEmpty(message.int_message_map)

    with self.assertRaises(TypeError):
      message.int_message_map['foo'].value = 5  # invalid key.

    with self.assertRaises(ValueError):
      message.int_message_map[1] = test_pb2.IntMessage()
    with self.assertRaises(AttributeError):
      message.int_message_map = {1: test_pb2.IntMessage()}

  def test_access_enum(self):
    message = m.make_test_message()

    self.assertEqual(message.enum_value, 0)

    message.enum_value = 1
    self.assertEqual(message.enum_value, test_pb2.TestMessage.ONE)

    message.enum_value = test_pb2.TestMessage.TWO
    self.assertEqual(message.enum_value, 2)

  def test_access_repeated_enum(self):
    message = m.make_test_message()

    message.repeated_enum_value.append(test_pb2.TestMessage.TWO)
    message.repeated_enum_value.append(test_pb2.TestMessage.ONE)
    self.assertLen(message.repeated_enum_value, 2)
    self.assertEqual(message.repeated_enum_value[0], 2)
    self.assertEqual(message.repeated_enum_value[1], 1)
    self.assertEqual(str(message.repeated_enum_value), '[2, 1]')

    with self.assertRaises(AttributeError):
      message.repeated_enum_value = [1]
    with self.assertRaises(IndexError):
      print(message.repeated_enum_value[1000])

  def test_access_nonexistent_field(self):
    message = m.make_test_message()
    with self.assertRaises(AttributeError):
      _ = message.invalid_field  # get
    with self.assertRaises(AttributeError):
      message.invalid_field = 5  # set

  def test_invalid_field_assignment(self):
    # Message, repeated, and map fields cannot be set via assignment.
    message = m.make_test_message()
    with self.assertRaises(AttributeError):
      message.int_message = test_pb2.IntMessage()
    with self.assertRaises(AttributeError):
      message.repeated_int_value = []
    with self.assertRaises(AttributeError):
      message.repeated_int_message = []
    with self.assertRaises(AttributeError):
      message.repeated_enum_value = []
    with self.assertRaises(AttributeError):
      message.string_int_map = {}
    with self.assertRaises(AttributeError):
      message.int_message_map = {}

  def test_has_field(self):
    message = m.make_test_message()
    self.assertFalse(message.HasField('int_message'))
    message.int_message.value = 5
    self.assertTrue(message.HasField('int_message'))
    with self.assertRaises(ValueError):
      message.HasField('non_existent_field')
    self.assertFalse(message.HasField('test_oneof'))
    message.oneof_a = 5
    self.assertTrue(message.HasField('test_oneof'))

  def test_clear_field(self):
    message = m.make_test_message()
    message.int_message.value = 5
    self.assertTrue(message.HasField('int_message'))
    message.ClearField('int_message')
    self.assertFalse(message.HasField('int_message'))
    with self.assertRaises(ValueError):
      message.ClearField('non_existent_field')
    message.oneof_a = 5
    message.ClearField('test_oneof')
    self.assertFalse(message.HasField('test_oneof'))

  def test_which_one_of(self):
    message = m.make_test_message()
    with self.assertRaises(ValueError):
      message.WhichOneof('non_existent_field')
    self.assertIsNone(message.WhichOneof('test_oneof'))
    message.oneof_a = 5
    self.assertEqual(message.WhichOneof('test_oneof'), 'oneof_a')
    message.oneof_b = 6
    self.assertEqual(message.WhichOneof('test_oneof'), 'oneof_b')

  def test_keep_alive_message(self):
    message = m.make_test_message()
    field = message.int_message
    # message should be kept alive until field is also deleted.
    del message
    field.value = 5
    self.assertEqual(field.value, 5)

  def test_keep_alive_repeated(self):
    message = m.make_test_message()
    field = message.repeated_int_value
    # message should be kept alive until field is also deleted.
    del message
    field.append(5)
    self.assertEqual(field[0], 5)

  def test_keep_alive_map(self):
    message = m.make_test_message()
    field = message.string_int_map
    # message should be kept alive until field is also deleted.
    del message
    field['test'] = 5
    self.assertEqual(field['test'], 5)

  @parameterized.named_parameters(
      ('copy_native', copy.copy, get_py_message),
      ('deepcopy_native', copy.copy, get_py_message),
      ('copy_cast', copy.deepcopy, get_cpp_message),
      ('deepcopy_cast', copy.deepcopy, get_cpp_message),
  )
  def test_special_copy_methods(self, copier, make_proto):
    message = make_proto()
    message.int_value = 5
    message_copy = copier(message)
    self.assertEqual(message_copy.int_value, 5)
    message_copy.int_value = 7
    self.assertEqual(message.int_value, 5)

  def test_get_entry_class(self):
    message = m.make_test_message()
    # GetEntryClass is used like this in text_format.
    map_entry = message.string_int_map.GetEntryClass()(key='k', value=5)
    self.assertEqual(map_entry.key, 'k')
    self.assertEqual(map_entry.value, 5)

  def test_text_format_to_string(self):
    self.assertMultiLineEqual(
        text_format.MessageToString(get_cpp_message()), m.TEXT_FORMAT_MESSAGE)

  def test_text_format_parse(self):
    message = text_format.Parse(m.TEXT_FORMAT_MESSAGE, m.make_test_message())
    self.assertMultiLineEqual(
        text_format.MessageToString(message), m.TEXT_FORMAT_MESSAGE
    )

  def test_text_format_merge(self):
    message = text_format.Merge(m.TEXT_FORMAT_MESSAGE, m.make_test_message())
    self.assertMultiLineEqual(
        text_format.MessageToString(message), m.TEXT_FORMAT_MESSAGE
    )

  def test_proto_2_equal(self):
    self.assertProtoEqual(m.TEXT_FORMAT_MESSAGE,
                          m.make_test_message(m.TEXT_FORMAT_MESSAGE))

  def test_byte_size_clear(self):
    message = get_cpp_message()
    self.assertEqual(message.ByteSize(), 53)

  def test_clear(self):
    message = get_cpp_message()
    message.Clear()
    self.assertProtoEqual('', message)

  @parameterized.named_parameters(  #
      ('no_kwargs', {}),  #
      ('deterministic_false', {
          'deterministic': False
      }),  #
      ('deterministic_true', {
          'deterministic': True
      }),  #
  )
  def test_serialize_and_parse(self, kwargs):
    text = m.make_test_message(
        m.TEXT_FORMAT_MESSAGE).SerializeToString(**kwargs)
    message_copy = m.make_test_message()
    message_copy.ParseFromString(text)
    self.assertProtoEqual(m.TEXT_FORMAT_MESSAGE, message_copy)

  @parameterized.named_parameters(  #
      ('no_kwargs', {}),  #
      ('deterministic_false', {
          'deterministic': False
      }),  #
      ('deterministic_true', {
          'deterministic': True
      }),  #
  )
  def test_serialize_partial_and_merge(self, kwargs):
    text = m.make_test_message(
        m.TEXT_FORMAT_MESSAGE).SerializePartialToString(**kwargs)
    message_copy = m.make_test_message()
    message_copy.MergeFromString(text)
    self.assertProtoEqual(m.TEXT_FORMAT_MESSAGE, message_copy)

  @parameterized.named_parameters(
      ('native_to_native', get_py_message, test_pb2.TestMessage),
      ('native_to_cast', get_py_message, m.make_test_message),
      ('cast_to_native', get_cpp_message, test_pb2.TestMessage),
      ('cast_to_cast', get_cpp_message, m.make_test_message),
  )
  def test_copy_from(self, a, b):
    source = a()
    dest = b()
    dest.CopyFrom(source)
    self.assertProtoEqual(m.TEXT_FORMAT_MESSAGE, dest)

  @parameterized.named_parameters(
      ('native_to_native', get_py_message, test_pb2.TestMessage),
      ('native_to_cast', get_py_message, m.make_test_message),
      ('cast_to_native', get_cpp_message, test_pb2.TestMessage),
      ('cast_to_cast', get_cpp_message, m.make_test_message),
  )
  def test_merge_from(self, a, b):
    source = a()
    dest = b()
    dest.MergeFrom(source)
    self.assertProtoEqual(m.TEXT_FORMAT_MESSAGE, dest)

  @parameterized.named_parameters(
      ('string', 'a string'),
      ('number', 1),
  )
  def test_copy_merge_from_invalid(self, other):
    message = get_cpp_message()
    with self.assertRaises(TypeError):
      message.CopyFrom(other)
    with self.assertRaises(TypeError):
      message.MergeFrom(other)

  @parameterized.named_parameters(
      ('cast', get_cpp_message),
      ('native', get_py_message),)
  def test_pickle_roundtrip(self, factory):
    message = factory()
    pickled = pickle.dumps(message, pickle.HIGHEST_PROTOCOL)
    restored = pickle.loads(pickled)
    self.assertProtoEqual(restored, message)

  @parameterized.named_parameters([
      ('native_any_proto', any_pb2.Any, test_pb2.IntMessage),
      ('native_any_proto_pybind_msg', any_pb2.Any, m.make_int_message)
  ])
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
  absltest.main()
