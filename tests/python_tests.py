import json
import unittest

import archspec_inspect


class PythonApiTests(unittest.TestCase):
    def test_native_json_and_mapping_api(self):
        raw = archspec_inspect.collect_json("os", include_sensitive=False)
        report = json.loads(raw)
        self.assertEqual(report["collected_categories"], ["os"])
        self.assertEqual(report["data"]["os"]["hostname"]["status"], "redacted")

        report = archspec_inspect.collect(["cpu"])
        self.assertEqual(report["collected_categories"], ["cpu"])
        self.assertIn("cpu", report["data"])
        self.assertIsInstance(archspec_inspect.version(), str)

    def test_invalid_categories_are_clear(self):
        with self.assertRaisesRegex(ValueError, "unknown category"):
            archspec_inspect.collect_json(["not-a-category"])
        with self.assertRaisesRegex(ValueError, "must not be empty"):
            archspec_inspect.collect_json([])


if __name__ == "__main__":
    unittest.main()
