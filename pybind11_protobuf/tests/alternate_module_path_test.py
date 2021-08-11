"""Test that an alternative module path ."""

from absl.testing import absltest

from pybind11_protobuf.tests import alternate_module_path_example


class AlternateModulePathTest(absltest.TestCase):

  def test_type(self):
    self.assertNotEqual(
        str(type(alternate_module_path_example.make_int_message())),
        "<class 'alternate_protobuf_module.ProtoMessage'>")


if __name__ == "__main__":
  absltest.main()
