"""Test that the caster imports the proto module automatically.

This must be a separate test from the proto_test because that test explicitly
imports and uses the proto module. Therfore it's impossible to verify that the
proto module is automatically imported correctly from that test. Instead we must
create a separate test which does *not* explicitly import the proto module.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from pybind11_protobuf import proto_example
from google3.testing.pybase import googletest


class ProtoAutoImportTest(googletest.TestCase):

  def test_access_wrapped_message(self):
    message = proto_example.make_test_message()
    message.string_value = 'test'
    message.int_value = 5
    message.int_message.value = 6
    self.assertEqual(message.string_value, 'test')
    self.assertEqual(message.int_value, 5)
    self.assertEqual(message.int_message.value, 6)


if __name__ == '__main__':
  googletest.main()
