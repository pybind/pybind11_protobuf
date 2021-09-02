"""Test pybind11_protobuf with multiple threads.

Run with `blaze test :thread_test --config=tsan`.
"""

import concurrent.futures

from absl.testing import absltest
from absl.testing import parameterized
from pybind11_protobuf.tests import thread_module


def make_message(x):
  return thread_module.make_message(x)


def make_message_string_view(x):
  return thread_module.make_message_string_view(x)


def make_message_no_gil(x):
  return thread_module.make_message_no_gil(x)


def make_message_string_view_no_gil(x):
  return thread_module.make_message_string_view_no_gil(x)


class ThreadTest(parameterized.TestCase):

  @parameterized.named_parameters(
      ('make_message', make_message),
      ('make_message_string_view', make_message_string_view),
      ('make_message_no_gil', make_message_no_gil),
      # BUG: https://github.com/pybind/pybind11/issues/2765
      # The following fails due to std::string_view casting in pybind11
      # ('make_message_string_view_no_gil', make_message_string_view_no_gil),
  )
  def test_parallel(self, fn):
    fn('a')

    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
      results = list(executor.map(fn, ['abc'] * 10))

    for x in results:
      self.assertEqual('abc', x.string_value)


if __name__ == '__main__':
  absltest.main()
