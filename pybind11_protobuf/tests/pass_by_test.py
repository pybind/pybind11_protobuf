# Copyright (c) 2021 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from google3.testing.pybase import googletest
from google3.testing.pybase import parameterized
from pybind11_protobuf.tests import pass_by_module as m
from pybind11_protobuf.tests import test_pb2


def get_abstract_pass_by_params():
  p = []
  for x in [
      'abstract_cptr',
      'abstract_cptr_notnone',
      'abstract_cref',
      'abstract_csptr',
      'abstract_cwref',
      'abstract_ptr',
      'abstract_ptr_notnone',
      'abstract_ref',
      'abstract_rval',
      'abstract_sptr',
      'abstract_uptr',
      'abstract_wref',
  ]:
    try:
      p.append((x, getattr(m, x)))
    except AttributeError:
      pass
  return p


def get_pass_by_params():
  p = get_abstract_pass_by_params()
  for x in [
      'concrete',
      'concrete_cptr',
      'concrete_cptr_notnone',
      'concrete_cref',
      'concrete_csptr',
      'concrete_cwref',
      'concrete_ptr',
      'concrete_ptr_notnone',
      'concrete_ref',
      'concrete_rval',
      'concrete_sptr',
      'concrete_uptr',
      'concrete_wref',
  ]:
    try:
      p.append((x, getattr(m, x)))
    except AttributeError:
      pass
  return p


class PassByTest(parameterized.TestCase):

  @parameterized.named_parameters(get_pass_by_params())
  def test_cpp_proto_check(self, check_method):
    message = m.make_int_message(7)
    self.assertTrue(check_method(message, 7))

  @parameterized.named_parameters(get_pass_by_params())
  def test_py_proto_check(self, check_method):
    message = test_pb2.IntMessage(value=8)
    self.assertTrue(check_method(message, 8))

  def test_pass_none(self):
    self.assertFalse(m.concrete_cptr(None, 1))
    self.assertFalse(m.abstract_cptr(None, 2))
    with self.assertRaises(TypeError):
      m.concrete_cptr_notnone(None, 3)
    with self.assertRaises(TypeError):
      m.abstract_cptr_notnone(None, 4)


if __name__ == '__main__':
  googletest.main()
