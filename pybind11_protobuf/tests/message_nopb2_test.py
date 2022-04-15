"""Protos can be returned even when no Python code depends on Python protobufs."""

from absl.testing import absltest
from pybind11_protobuf.tests import message_module as m


class MessageNopb2Test(absltest.TestCase):

  def test_return_message(self):
    msg = m.make_test_message('int_value: 4')
    fields = msg.ListFields()
    self.assertLen(fields, 1)
    field_desc, value = fields[0]
    self.assertEqual(field_desc.name, 'int_value')
    self.assertEqual(value, 4)


if __name__ == '__main__':
  absltest.main()
