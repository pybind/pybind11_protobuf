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

from pybind11_protobuf.tests import compare
from pybind11_protobuf.tests import test_pb2
from pybind11_protobuf.tests import wrapped_proto_module as m


class FastProtoTest(parameterized.TestCase, compare.ProtoAssertions):

  def test_type(self):
    # These are both seen as the concrete type.
    self.assertIn(
        str(type(m.get_test_message())), [
            "<class 'pybind11_protobuf.tests.test_pb2.TestMessage'>",
        ])

  def test_full_name(self):
    self.assertEqual(
        str(m.make_int_message().DESCRIPTOR.full_name),
        'pybind11.test.IntMessage')

  def test_str(self):
    a = m.get_test_message()
    b = m.get_test_message()
    self.assertEqual(str(a), str(b))

  def test_readable(self):
    a = m.get_test_message()
    self.assertEqual(123, a.int_value)

  @parameterized.named_parameters(
      ('check', m.check),
      ('check_cref', m.check_cref),
      ('check_cptr', m.check_cptr),
      ('check_val', m.check_val),
      ('check_rval', m.check_rval),
      ('check_mutable', m.check_mutable),
  )
  def test_native(self, check):
    a = test_pb2.TestMessage()
    a.int_value = 33
    self.assertTrue(check(a, 33))

  @parameterized.named_parameters(
      ('check', m.check),
      ('check_cref', m.check_cref),
      ('check_cptr', m.check_cptr),
      ('check_val', m.check_val),
      ('check_rval', m.check_rval),
      ('check_mutable', m.check_mutable),
  )
  def test_wrapped(self, check):
    a = m.get_test_message()
    a.int_value = 34
    self.assertTrue(check(a, 34))

  def test_call_with_str(self):
    with self.assertRaises(TypeError):
      m.check('any string', 32)

  def test_call_with_none(self):
    self.assertFalse(m.check(None, 32))

  def test_equality(self):
    a = m.get_test_message()
    a.int_value = 44
    self.assertProtoEqual(test_pb2.TestMessage(int_value=44), a)

  def test_fn_overload(self):
    self.assertEqual(1, m.fn_overload(test_pb2.IntMessage()))
    self.assertEqual(1, m.fn_overload(m.make_int_message()))
    with self.assertRaises(TypeError):
      m.fn_overload('any string')


if __name__ == '__main__':
  absltest.main()
