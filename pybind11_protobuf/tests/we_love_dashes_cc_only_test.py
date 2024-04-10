# Copyright (c) 2024 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.

from absl.testing import absltest
from google.protobuf.internal import api_implementation
from pybind11_protobuf.tests import we_love_dashes_module


class MessageTest(absltest.TestCase):

  def test_return_then_pass(self):
    if api_implementation.Type() == 'cpp':
      msg = we_love_dashes_module.return_token_effort(234)
      score = we_love_dashes_module.pass_token_effort(msg)
      self.assertEqual(score, 234)
    else:
      with self.assertRaisesRegex(
          TypeError,
          r'^Cannot construct a protocol buffer message type'
          r' pybind11\.test\.TokenEffort in python\.'
          r' .*pybind11_protobuf\.tests\.we_love_dashes_pb2\?$',
      ):
        we_love_dashes_module.return_token_effort(0)


if __name__ == '__main__':
  absltest.main()
