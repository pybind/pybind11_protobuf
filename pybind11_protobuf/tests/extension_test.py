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

from google.protobuf.internal import api_implementation
from pybind11_protobuf.tests import extension_in_other_file_in_deps_pb2
from pybind11_protobuf.tests import extension_in_other_file_pb2
from pybind11_protobuf.tests import extension_module as m
from pybind11_protobuf.tests import extension_nest_repeated_pb2
from pybind11_protobuf.tests import extension_pb2


def get_py_message(value=5,
                   in_other_file_in_deps_value=None,
                   in_other_file_value=None):
  """Returns a dynamic message that is wire-compatible with IntMessage."""
  msg = extension_pb2.BaseMessage()
  if in_other_file_in_deps_value is not None:
    msg.Extensions[
        extension_in_other_file_in_deps_pb2.MessageInOtherFileInDeps.
        message_in_other_file_in_deps_extension].value = in_other_file_in_deps_value
  elif in_other_file_value is not None:
    msg.Extensions[extension_in_other_file_pb2.MessageInOtherFile
                   .message_in_other_file_extension].value = in_other_file_value
  else:
    msg.Extensions[extension_pb2.int_message].value = value
  return msg


def get_allow_unknown_inner(value):
  msg = extension_pb2.AllowUnknownInner()
  msg.Extensions[
      extension_in_other_file_pb2.AllowUnknownInnerExtension.hook].value = value
  return msg


class ExtensionTest(parameterized.TestCase):

  @parameterized.named_parameters(('python', get_py_message),
                                  ('cpp', m.get_message))
  def test_full_name(self, get_message_function):
    a = get_message_function()
    self.assertEqual(str(a.DESCRIPTOR.full_name), 'pybind11.test.BaseMessage')

  @parameterized.named_parameters(('python', get_py_message),
                                  ('cpp', m.get_message))
  def test_get_extension_py(self, get_message_function):
    a = get_message_function(value=8)
    self.assertEqual(8, a.Extensions[extension_pb2.int_message].value)

  @parameterized.named_parameters(('python', get_py_message),
                                  ('cpp', m.get_message))
  def test_set_extension_cpp(self, get_message_function):
    a = get_message_function()
    a.Extensions[extension_pb2.int_message].value = 8
    self.assertEqual(8, a.Extensions[extension_pb2.int_message].value)

  @parameterized.named_parameters(('python', get_py_message),
                                  ('cpp', m.get_message))
  def test_extension_fn(self, get_message_function):
    a = get_message_function(value=7)
    b = m.get_extension(a)
    self.assertEqual(7, b.value)

  @parameterized.named_parameters(('python', get_py_message),
                                  ('cpp', m.get_message))
  def test_extension_fn_roundtrip(self, get_message_function):
    a = get_message_function(value=7)
    b = m.get_extension(m.roundtrip(a))
    self.assertEqual(7, b.value)

  @parameterized.parameters('roundtrip', 'reserialize_base_message')
  def test_extension_in_same_file(self, roundtrip_fn):
    a = get_py_message(94)
    b = getattr(m, roundtrip_fn)(a)
    self.assertEqual(94, b.Extensions[extension_pb2.int_message].value)

  @parameterized.parameters('roundtrip', 'reserialize_base_message')
  def test_extension_in_other_file_in_deps(self, roundtrip_fn):
    a = get_py_message(in_other_file_in_deps_value=38)
    b = getattr(m, roundtrip_fn)(a)
    self.assertEqual(
        38, b.Extensions[
            extension_in_other_file_in_deps_pb2.MessageInOtherFileInDeps
            .message_in_other_file_in_deps_extension].value)

  def test_extension_in_other_file_roundtrip(self):
    a = get_py_message(in_other_file_value=57)
    b = m.roundtrip(a)
    self.assertEqual(
        57, b.Extensions[extension_in_other_file_pb2.MessageInOtherFile
                         .message_in_other_file_extension].value)

  def test_reserialize_base_message(self):
    a = get_py_message(in_other_file_value=63)
    if api_implementation.Type() == 'cpp':
      with self.assertRaises(ValueError) as ctx:
        m.reserialize_base_message(a)
      self.assertStartsWith(
          str(ctx.exception),
          'Proto Message of type pybind11.test.BaseMessage has an'
          ' Unknown Field: 1003 (')
      self.assertEndsWith(
          str(ctx.exception),
          'extension.proto). Please add the required `cc_proto_library` `deps`.'
          ' Only if there is no alternative to suppressing this error, use'
          ' `pybind11_protobuf::AllowUnknownFieldsFor('
          '"pybind11.test.BaseMessage", "");`'
          ' (Warning: suppressions may mask critical bugs.)')
    else:
      b = m.reserialize_base_message(a)
      b_value = b.Extensions[extension_in_other_file_pb2.MessageInOtherFile
                             .message_in_other_file_extension].value
      self.assertEqual(63, b_value)

  def test_reserialize_nest_level2(self):
    a = extension_pb2.NestLevel2(
        nest_lvl1=extension_pb2.NestLevel1(
            base_msg=get_py_message(in_other_file_value=52)))
    if api_implementation.Type() == 'cpp':
      with self.assertRaises(ValueError) as ctx:
        m.reserialize_nest_level2(a)
      self.assertStartsWith(
          str(ctx.exception),
          'Proto Message of type pybind11.test.NestLevel2 has an Unknown Field'
          ' with parent of type pybind11.test.BaseMessage:'
          ' nest_lvl1.base_msg.1003 (')
      self.assertEndsWith(
          str(ctx.exception),
          'extension.proto). Please add the required `cc_proto_library` `deps`.'
          ' Only if there is no alternative to suppressing this error, use'
          ' `pybind11_protobuf::AllowUnknownFieldsFor('
          '"pybind11.test.NestLevel2", "nest_lvl1.base_msg");`'
          ' (Warning: suppressions may mask critical bugs.)')
    else:
      b = m.reserialize_nest_level2(a)
      b_value = b.nest_lvl1.base_msg.Extensions[
          extension_in_other_file_pb2.MessageInOtherFile
          .message_in_other_file_extension].value
      self.assertEqual(52, b_value)

  def test_reserialize_nest_repeated(self):
    a = extension_nest_repeated_pb2.NestRepeated(base_msgs=[
        get_py_message(in_other_file_value=74),
        get_py_message(in_other_file_value=85)
    ])
    if api_implementation.Type() == 'cpp':
      with self.assertRaises(ValueError) as ctx:
        m.reserialize_nest_repeated(a)
      self.assertStartsWith(
          str(ctx.exception), 'Proto Message of type pybind11.test.NestRepeated'
          ' has an Unknown Field'
          ' with parent of type pybind11.test.BaseMessage: base_msgs.1003 (')
      self.assertIn('extension_nest_repeated.proto, ', str(ctx.exception))
      self.assertEndsWith(
          str(ctx.exception),
          'extension.proto). Please add the required `cc_proto_library` `deps`.'
          ' Only if there is no alternative to suppressing this error, use'
          ' `pybind11_protobuf::AllowUnknownFieldsFor('
          '"pybind11.test.NestRepeated", "base_msgs");`'
          ' (Warning: suppressions may mask critical bugs.)')
    else:
      b = m.reserialize_nest_repeated(a)
      for bm, expected_b_value in zip(b.base_msgs, (74, 85)):
        b_value = bm.Extensions[extension_in_other_file_pb2.MessageInOtherFile
                                .message_in_other_file_extension].value
        self.assertEqual(expected_b_value, b_value)

  def test_reserialize_allow_unknown_inner(self):
    a = get_allow_unknown_inner(96)
    b = m.reserialize_allow_unknown_inner(a)
    if api_implementation.Type() == 'cpp':
      self.assertLen(b.UnknownFields(), 1)
      self.assertEqual(2001, b.UnknownFields()[0].field_number)
    else:
      b_value = b.Extensions[
          extension_in_other_file_pb2.AllowUnknownInnerExtension.hook].value
      self.assertEqual(96, b_value)

  def test_reserialize_allow_unknown_outer(self):
    a = extension_pb2.AllowUnknownOuter(inner=get_allow_unknown_inner(97))
    b = m.reserialize_allow_unknown_outer(a)
    if api_implementation.Type() == 'cpp':
      self.assertLen(b.inner.UnknownFields(), 1)
      self.assertEqual(2001, b.inner.UnknownFields()[0].field_number)
    else:
      b_inner_value = b.inner.Extensions[
          extension_in_other_file_pb2.AllowUnknownInnerExtension.hook].value
      self.assertEqual(97, b_inner_value)


if __name__ == '__main__':
  absltest.main()
