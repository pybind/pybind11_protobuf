"""Test that importing `proto_example` before `proto` works.

Those modules import the same bindings via slightly different mechanisms,
so the import order tests confirm that it does not matter which of those
mechanisms is used first- the result is the same.
"""

from pybind11_protobuf.tests import proto_example
from pybind11_protobuf import proto  # pylint: disable=g-bad-import-order
import unittest


class ImportOrderBTest(unittest.TestCase):

  def test_import(self):
    self.assertTrue(proto.is_wrapped_c_proto(proto_example.make_test_message()))


if __name__ == '__main__':
  unittest.main()
