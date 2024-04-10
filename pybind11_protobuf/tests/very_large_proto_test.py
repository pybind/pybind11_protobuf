# Copyright (c) 2023 The Pybind Development Team. All rights reserved.
#
# All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.

from absl.testing import absltest

from pybind11_protobuf.tests import pass_proto2_message_module
from pybind11_protobuf.tests import test_pb2


class MessageTest(absltest.TestCase):

  def test_greater_than_2gb_limit(self):
    # This test is expected to fail if the Python-to-C++ conversion involves
    # serialization/deserialization.
    # Code exercised: IsCProtoSuitableForCopyFromCall()
    kb = 1024
    msg_size = 2 * kb**3 + kb  # A little over 2 GB.
    msg = test_pb2.TestMessage(string_value='x' * msg_size)
    space_used_estimate = pass_proto2_message_module.get_space_used_estimate(
        msg
    )
    self.assertGreater(space_used_estimate, msg_size)


if __name__ == '__main__':
  absltest.main()
