"""Tests for protobuf casters."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from google3.testing.pybase import googletest
from pybind11_protobuf import proto
from pybind11_protobuf import proto_example
from pybind11_protobuf import test_pb2


class ProtoTest(googletest.TestCase):

  def test_return_wrapped_message(self):
    message = proto_example.make_test_message()
    self.assertEqual(message.DESCRIPTOR.full_name, 'pybind11.test.TestMessage')
    self.assertTrue(proto.is_wrapped_c_proto(message))
    self.assertFalse(proto.is_wrapped_c_proto('not a proto'))
    self.assertFalse(proto.is_wrapped_c_proto(test_pb2.TestMessage()))

  def test_access_wrapped_message_singluar_fields(self):
    message = proto_example.make_test_message()
    message.string_value = 'test'
    message.int_value = 5
    message.int_message.value = 6
    self.assertEqual(message.string_value, 'test')
    self.assertEqual(message.int_value, 5)
    self.assertEqual(message.int_message.value, 6)

  def test_access_wrapped_message_repeated_int_value(self):
    message = proto_example.make_test_message()
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

    message.repeated_int_value.remove(1)
    self.assertSequenceEqual(message.repeated_int_value, [8, 7])

    message.repeated_int_value.extend([6, 5])
    self.assertSequenceEqual(message.repeated_int_value, [8, 7, 6, 5])

    message.repeated_int_value[1] = 5
    self.assertSequenceEqual(message.repeated_int_value, [8, 5, 6, 5])

    message.repeated_int_value.clear()
    self.assertEmpty(message.repeated_int_value)

    self.assertRaises(TypeError, message.repeated_int_value.append,
                      'invalid value')

  def test_access_wrapped_message_repeated_int_message(self):
    message = proto_example.make_test_message()
    sub_msg = proto_example.make_int_message()
    sub_msg.value = 6
    message.repeated_int_message.append(sub_msg)
    # Append/Extend/Set should work from native or wrapped messages.
    sub_msg = test_pb2.IntMessage()
    sub_msg.value = 7
    message.repeated_int_message.append(sub_msg)

    def check_values(values):
      for msg, expected in zip(message.repeated_int_message, values):
        self.assertEqual(msg.value, expected)

    self.assertLen(message.repeated_int_message, 2)
    self.assertEqual(message.repeated_int_message[0].value, 6)
    self.assertEqual(message.repeated_int_message[1].value, 7)
    check_values([6, 7])

    self.assertEqual(str(message.repeated_int_message), '[value: 6, value: 7]')

    message.repeated_int_message[0].value = 8
    check_values([8, 7])

    sub_msg.value = 2
    message.repeated_int_message.insert(1, sub_msg)
    check_values([8, 2, 7])

    message.repeated_int_message.extend([sub_msg, sub_msg])
    check_values([8, 2, 7, 2, 2])

    message.repeated_int_message.clear()
    self.assertEmpty(message.repeated_int_message)

    self.assertRaises(ValueError, message.repeated_int_message.append,
                      'invalid value')
    self.assertRaises(ValueError, message.repeated_int_message.append,
                      test_pb2.TestMessage())

    # TODO(kenoslund): Get remove to work.
    self.assertRaises(RuntimeError, message.repeated_int_message.remove, 1)

  def test_access_wrapped_message_map_string_int(self):
    message = proto_example.make_test_message()

    message.string_int_map['k1'] = 5
    message.string_int_map['k2'] = 6
    self.assertLen(message.string_int_map, 2)
    self.assertEqual(message.string_int_map['k1'], 5)
    self.assertEqual(message.string_int_map['k2'], 6)
    self.assertIn('k1', message.string_int_map)
    self.assertIn('k2', message.string_int_map)
    self.assertNotIn('k3', message.string_int_map)

    native = test_pb2.TestMessage()
    native.string_int_map['k1'] = 5
    native.string_int_map['k2'] = 6
    self.assertEqual(str(message.string_int_map), str(native.string_int_map))

    message.string_int_map.clear()
    self.assertEmpty(message.string_int_map)

    with self.assertRaises(TypeError):
      message.string_int_map[5] = 5  # invalid key.
    with self.assertRaises(TypeError):
      message.string_int_map['k'] = 'foo'  # invalid value.

  def test_access_wrapped_message_map_int_message(self):
    message = proto_example.make_test_message()

    message.int_message_map[5].value = 2
    message.int_message_map[6].value = 3
    self.assertLen(message.int_message_map, 2)
    self.assertEqual(message.int_message_map[5].value, 2)
    self.assertEqual(message.int_message_map[6].value, 3)
    self.assertIn(5, message.int_message_map)
    self.assertIn(6, message.int_message_map)
    self.assertNotIn(7, message.int_message_map)

    self.assertEqual(str(message.int_message_map), '{5: value: 2, 6: value: 3}')

    message.int_message_map.clear()
    self.assertEmpty(message.int_message_map)

    with self.assertRaises(TypeError):
      message.int_message_map['foo'].value = 5  # invalid key.

  def test_access_wrapped_message_enum(self):
    message = proto_example.make_test_message()

    self.assertEqual(message.enum_value, 0)

    message.enum_value = 1
    self.assertEqual(message.enum_value, test_pb2.TestMessage.ONE)

    message.enum_value = test_pb2.TestMessage.TWO
    self.assertEqual(message.enum_value, 2)

  def test_access_wrapped_message_repeated_enum(self):
    message = proto_example.make_test_message()

    message.repeated_enum_value.append(test_pb2.TestMessage.TWO)
    message.repeated_enum_value.append(test_pb2.TestMessage.ONE)
    self.assertLen(message.repeated_enum_value, 2)
    self.assertEqual(message.repeated_enum_value[0], 2)
    self.assertEqual(message.repeated_enum_value[1], 1)
    self.assertEqual(str(message.repeated_enum_value), '[TWO, ONE]')

  def test_pass_wrapped_proto(self):
    message = proto_example.make_test_message()
    message.int_value = 5
    self.assertTrue(proto_example.check_test_message(message, 5))

  def test_pass_wrapped_proto_wrong_type(self):
    message = proto_example.make_int_message()
    self.assertRaises(TypeError, proto_example.check_test_message, message, 5)

  def test_pass_native_proto(self):
    message = test_pb2.TestMessage()
    message.int_value = 5
    self.assertTrue(proto_example.check_test_message(message, 5))

  def test_pass_native_proto_wrong_type(self):
    message = test_pb2.IntMessage()
    self.assertRaises(TypeError, proto_example.check_test_message, message, 5)

  def test_pass_not_a_proto(self):
    self.assertRaises(TypeError, proto_example.check_test_message,
                      'not_a_proto', 5)

  def test_mutate_wrapped_proto(self):
    message = proto_example.make_test_message()
    proto_example.mutate_test_message(5, message)
    self.assertEqual(message.int_value, 5)

  def test_mutate_native_proto(self):
    # This is not allowed (enforce by `.noconvert()` on the argument binding).
    message = test_pb2.TestMessage()
    self.assertRaises(TypeError, proto_example.mutate_test_message, 5, message)

  def test_pass_generic_wrapped_proto(self):
    message = proto_example.make_test_message()
    self.assertTrue(
        proto_example.check_generic_message(message,
                                            message.DESCRIPTOR.full_name))

  def test_pass_generic_native_proto(self):
    message = test_pb2.TestMessage()
    self.assertTrue(
        proto_example.check_generic_message(message,
                                            message.DESCRIPTOR.full_name))

  def test_make_any_from_wrapped_proto(self):
    message = proto_example.make_test_message()
    message.int_value = 5
    any_proto = proto_example.make_any_message(message)
    self.assertEqual(any_proto.type_url,
                     'type.googleapis.com/pybind11.test.TestMessage')
    self.assertEqual(any_proto.value, b'\x10\x05')

  def test_make_any_from_native_proto(self):
    message = test_pb2.TestMessage()
    message.int_value = 5
    any_proto = proto_example.make_any_message(message)
    self.assertEqual(any_proto.type_url,
                     'type.googleapis.com/pybind11.test.TestMessage')
    self.assertEqual(any_proto.value, b'\x10\x05')


if __name__ == '__main__':
  googletest.main()
