# Copyright (c) 2019 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.
"""Tests for protobuf casters."""

from __future__ import absolute_import

from absl.testing import absltest

from pybind11_protobuf.tests import proto_enum_module as m
from pybind11_protobuf.tests import test_pb2


class ProtoEnumTest(absltest.TestCase):

  def test_enum(self):
    self.assertEqual(m.adjust_enum(0), 1)
    self.assertEqual(
        m.adjust_enum(test_pb2.TestMessage.ONE), test_pb2.TestMessage.TWO)
    self.assertEqual(m.adjust_enum(m.ZERO), m.ONE)
    with self.assertRaises(ValueError):
      m.adjust_enum(7)
    with self.assertRaises(TypeError):
      m.adjust_enum('ZERO')

  def test_another_enum(self):
    self.assertEqual(m.adjust_another_enum(m.AnotherEnum.ZERO), 11)
    self.assertEqual(
        m.adjust_another_enum(m.AnotherEnum.ONE), test_pb2.AnotherMessage.TWO)
    self.assertEqual(
        m.adjust_another_enum(m.AnotherEnum.TWO), test_pb2.AnotherMessage.ZERO)
    # py::enum_ conversion of proto::Enum is more strict than the type_caster<>
    # conversion.
    with self.assertRaises(TypeError):
      m.adjust_another_enum(test_pb2.AnotherMessage.ZERO)
    with self.assertRaises(TypeError):
      m.adjust_another_enum(0)
    with self.assertRaises(TypeError):
      m.adjust_enum('ZERO')


if __name__ == '__main__':
  absltest.main()
