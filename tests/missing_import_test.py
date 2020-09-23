"""Tests for google3.third_party.pybind11_protobuf.tests.missing_import."""

import unittest
from pybind11_protobuf.tests import missing_import
from pybind11_protobuf.tests import test_pb2


class MissingImportTest(unittest.TestCase):

  message_regex = 'Proto module has not been imported.*'

  def test_returns_proto(self):
    with self.assertRaisesRegex(TypeError, self.message_regex):
      missing_import.returns_proto()

  def test_takes_proto(self):
    proto = test_pb2.IntMessage()
    with self.assertRaisesRegex(TypeError, self.message_regex):
      missing_import.takes_proto(proto)


if __name__ == '__main__':
  unittest.main()
