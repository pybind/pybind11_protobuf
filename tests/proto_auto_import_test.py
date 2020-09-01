"""Test that the caster imports the proto module automatically.

This must be a separate test from the proto_test because that test explicitly
imports and uses the proto module. Therfore it's impossible to verify that the
proto module is automatically imported correctly from that test. Instead we must
create a separate test which does *not* explicitly import the proto module.

Additionally, this test only calls a function which returns a pointer to an
abstract message (ie, a proto2::Message pointer) rather than a concrete message
type (eg, IntMessage, TestMessage, etc), so we can test that everything works
without the concrete type being registered.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from pybind11_protobuf.tests import proto_example
import unittest


class ProtoAutoImportTest(unittest.TestCase):

  def test_access_abstract_message(self):
    # Test setting and getting a field in a message which is not registered
    # as a concrete type (only an abstract one).
    message = proto_example.make_int_message_as_abstract()
    message.value = 5
    self.assertEqual(message.value, 5)

  def test_call_constructor_registered_type(self):
    # TestMessage was explicitly registered in proto_example.cc, so it's
    # registered as a concrete type even though it has not been returned as
    # such. Therefore it should have a constructor.
    message1 = proto_example.make_test_message_as_abstract()
    message2 = message1.__class__(int_value=5)
    self.assertEqual(message2.int_value, 5)

  def test_call_constructor_unregistered_type(self):
    # IntMessage has never been registered, either in proto_example.cc or by
    # calling a function which returns it as a concrete type, so its __class__
    # property corresponds to the abstract type and doesn't have a constructor.
    # Therefore attempting to call the constructor should fail.
    message = proto_example.make_int_message_as_abstract()
    with self.assertRaises(TypeError):
      _ = message.__class__(value=5)


if __name__ == '__main__':
  unittest.main()
