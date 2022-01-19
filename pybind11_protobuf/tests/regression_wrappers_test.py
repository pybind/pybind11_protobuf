# Copyright (c) 2022 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.
"""Tests for protobuf casters."""

from __future__ import absolute_import

from absl.testing import absltest
from absl.testing import parameterized

from google.protobuf import any_pb2
from google.protobuf import wrappers_pb2
from pybind11_protobuf.tests import regression_wrappers_module as m


class RegressionWrappersTest(parameterized.TestCase):

  @parameterized.named_parameters(('bool_value', wrappers_pb2.BoolValue),
                                  ('string_value', wrappers_pb2.StringValue),
                                  ('any', any_pb2.Any))
  def test_print_descriptor_unique_ptr(self, get_message_function):
    message = get_message_function()
    self.assertIn(message.DESCRIPTOR.name,
                  m.print_descriptor_unique_ptr(message))


if __name__ == '__main__':
  absltest.main()
