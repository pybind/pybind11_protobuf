"""Test that an alternative module path ."""

from pybind11_protobuf.tests import alternate_module_path_example
from google3.testing.pybase import googletest


class AlternateModulePathTest(googletest.TestCase):

  def test_type(self):
    self.assertEqual(
        str(type(alternate_module_path_example.make_int_message())),
        "<class 'alternate_protobuf_module.ProtoMessage'>")


if __name__ == "__main__":
  googletest.main()
