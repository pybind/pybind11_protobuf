# Copyright (c) 2024 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.

from absl.testing import absltest
from pybind11_protobuf.tests import we_love_dashes_cc_only_module

# NOTE: ":we-love-dashes_py_pb2" is in deps but intentionally not imported here.


class MessageTest(absltest.TestCase):

  def test_return_then_pass(self):
    msg = we_love_dashes_cc_only_module.return_token_effort(234)
    score = we_love_dashes_cc_only_module.pass_token_effort(msg)
    self.assertEqual(score, 234)


if __name__ == '__main__':
  absltest.main()
