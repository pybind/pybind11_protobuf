# Copyright (c) 2024 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.

from absl.testing import absltest

from pybind11_protobuf.tests import very_large_proto_module
from pybind11_protobuf.tests import we_love_dashes_pb2


class MessageTest(absltest.TestCase):

  def test_pass_proto2_message(self):
    msg = we_love_dashes_pb2.TokenEffort(score=345)
    space_used_estimate = very_large_proto_module.get_space_used_estimate(msg)
    self.assertGreater(space_used_estimate, 0)


if __name__ == '__main__':
  absltest.main()
