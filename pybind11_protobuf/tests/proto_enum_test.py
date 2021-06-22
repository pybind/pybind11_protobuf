# Copyright (c) 2019 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.
"""Tests for protobuf casters."""

from __future__ import absolute_import

from google3.testing.pybase import googletest
from pybind11_protobuf.tests import proto_enum_module as m
from pybind11_protobuf.tests import test_pb2


class ProtoEnumTest(googletest.TestCase):

  def test_enum(self):
    self.assertEqual(m.adjust_enum(0), 1)
    self.assertEqual(
        m.adjust_enum(test_pb2.TestMessage.ONE), test_pb2.TestMessage.TWO)
    self.assertEqual(m.adjust_enum(m.ZERO), m.ONE)
    with self.assertRaises(ValueError):
      m.adjust_enum(7)


if __name__ == '__main__':
  googletest.main()
