# Copyright (c) 2021 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from absl.testing import absltest
from absl.testing import parameterized

from pybind11_protobuf.tests import pass_by_module as m
from pybind11_protobuf.tests import test_pb2
from google.protobuf import descriptor_pool
from google.protobuf import message_factory


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
      'concrete_csptr_ref',
      'concrete_cwref',
      'concrete_ptr',
      'concrete_ptr_notnone',
      'concrete_ref',
      'concrete_rval',
      'concrete_crval',
      'concrete_sptr',
      'concrete_uptr',
      'concrete_uptr_ptr',
      'concrete_uptr_ref',
      'concrete_wref',
      'std_variant',
      'std_optional,'
  ]:
    try:
      p.append((x, getattr(m, x)))
    except AttributeError:
      pass
  return p


class FakeDescriptor:

  def __init__(self):
    self.full_name = 'not.a.Message'


class FakeMessage:

  def __init__(self, value=1):
    self.DESCRIPTOR = FakeDescriptor()  # pylint: disable=invalid-name
    self.value = value


class PassByTest(parameterized.TestCase):

  @parameterized.named_parameters(
      ('make', m.make_int_message),
      ('make_ptr', m.make_ptr),
      ('make_sptr', m.make_sptr),
      ('make_uptr', m.make_uptr),
      ('static_cptr', m.static_cptr),
      ('static_cref', m.static_cref),
      ('static_ptr', m.static_ptr),
      ('static_ref', m.static_ref),
      ('static_ref_auto', m.static_ref_auto),
  )
  def test_construct(self, constructor):
    message = constructor()
    message.value = 5
    self.assertEqual(message.value, 5)

  @parameterized.named_parameters(get_pass_by_params())
  def test_cpp_proto_check(self, check_method):
    message = m.make_int_message(7)
    self.assertTrue(check_method(message, 7))

  @parameterized.named_parameters(get_pass_by_params())
  def test_py_proto_check(self, check_method):
    message = test_pb2.IntMessage(value=8)
    self.assertTrue(check_method(message, 8))

  @parameterized.named_parameters(get_pass_by_params())
  def test_pool_proto_check(self, check_method):
    pool = descriptor_pool.Default()
    factory = message_factory.MessageFactory(pool)
    prototype = factory.GetPrototype(
        pool.FindMessageTypeByName('pybind11.test.IntMessage'))
    message = prototype(value=9)
    self.assertTrue(check_method(message, 9))

  def test_pass_none(self):
    self.assertFalse(m.concrete_cptr(None, 1))
    self.assertFalse(m.abstract_cptr(None, 2))
    with self.assertRaises(TypeError):
      m.concrete_cptr_notnone(None, 3)
    with self.assertRaises(TypeError):
      m.abstract_cptr_notnone(None, 4)
    self.assertFalse(m.std_variant(None, 0))
    self.assertFalse(m.std_optional(None, 0))

  @parameterized.named_parameters(
      ('concrete', m.concrete_cref),
      ('abstract', m.abstract_cref),
      ('shared_ptr', m.concrete_sptr),
      ('unique_ptr', m.abstract_uptr),
  )
  def test_pass_string(self, check_method):
    with self.assertRaises(TypeError):
      check_method('string', 4)

  @parameterized.named_parameters(
      ('concrete', m.concrete_cref),
      ('abstract', m.abstract_cref),
      ('shared_ptr', m.concrete_sptr),
      ('unique_ptr', m.abstract_uptr),
  )
  def test_pass_fake1(self, check_method):
    fake = FakeMessage()
    with self.assertRaises(TypeError):
      check_method(fake, 4)

  @parameterized.named_parameters(
      ('concrete', m.concrete_cref),
      ('abstract', m.abstract_cref),
      ('shared_ptr', m.concrete_sptr),
      ('unique_ptr', m.abstract_uptr),
  )
  def test_pass_fake2(self, check_method):
    fake = FakeMessage()
    del fake.DESCRIPTOR.full_name
    with self.assertRaises(TypeError):
      check_method(fake, 4)

  @parameterized.named_parameters(
      ('make', m.make_int_message, 2),
      ('int_message', test_pb2.IntMessage, 2),
      ('test_message', test_pb2.TestMessage, 1),
  )
  def test_overload_fn(self, message_fn, expected):
    self.assertEqual(expected, m.fn_overload(message_fn()))


if __name__ == '__main__':
  absltest.main()
