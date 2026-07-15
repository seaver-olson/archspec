#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <archspec/archspec.hpp>

#include <exception>
#include <string>

namespace {

struct CategoryEntry {
  const char* name;
  archspec::CollectCategory value;
};

constexpr CategoryEntry kCategories[] = {
    {"os", archspec::CollectCategory::os},
    {"cpu", archspec::CollectCategory::cpu},
    {"isa", archspec::CollectCategory::isa},
    {"cache", archspec::CollectCategory::cache},
    {"memory", archspec::CollectCategory::memory},
    {"pci", archspec::CollectCategory::pci},
    {"gpu", archspec::CollectCategory::gpu},
    {"perf", archspec::CollectCategory::perf},
    {"block", archspec::CollectCategory::block},
    {"net", archspec::CollectCategory::net},
    {"thermal", archspec::CollectCategory::thermal},
    {"power", archspec::CollectCategory::power},
    {"virtualization", archspec::CollectCategory::virtualization},
    {"platform", archspec::CollectCategory::platform},
};

bool add_category(const std::string& name, archspec::CollectCategory& categories) {
  for (const CategoryEntry& entry : kCategories) {
    if (name == entry.name) {
      categories = categories | entry.value;
      return true;
    }
  }
  return false;
}

bool parse_category_name(
    const std::string& name,
    archspec::CollectCategory& categories
) {
  if (name.empty()) {
    PyErr_SetString(PyExc_ValueError, "category names must not be empty");
    return false;
  }
  if (name == "all") {
    categories = archspec::CollectCategory::all;
    return true;
  }
  if (!add_category(name, categories)) {
    PyErr_Format(PyExc_ValueError, "unknown category '%s'", name.c_str());
    return false;
  }
  return true;
}

bool parse_categories(PyObject* value, archspec::CollectCategory& categories) {
  categories = archspec::CollectCategory::all;
  if (value == nullptr || value == Py_None) {
    return true;
  }

  categories = archspec::CollectCategory::none;
  if (PyUnicode_Check(value)) {
    const char* text = PyUnicode_AsUTF8(value);
    if (text == nullptr) {
      return false;
    }
    std::string names(text);
    std::size_t start = 0;
    while (start <= names.size()) {
      const std::size_t comma = names.find(',', start);
      const std::string name = names.substr(
          start,
          comma == std::string::npos ? std::string::npos : comma - start
      );
      if (!parse_category_name(name, categories)) {
        return false;
      }
      if (comma == std::string::npos) {
        return true;
      }
      start = comma + 1;
    }
    return true;
  }

  PyObject* iterator = PyObject_GetIter(value);
  if (iterator == nullptr) {
    PyErr_SetString(PyExc_TypeError, "categories must be a string, iterable of strings, or None");
    return false;
  }
  PyObject* item = nullptr;
  while ((item = PyIter_Next(iterator)) != nullptr) {
    if (!PyUnicode_Check(item)) {
      Py_DECREF(item);
      Py_DECREF(iterator);
      PyErr_SetString(PyExc_TypeError, "each category must be a string");
      return false;
    }
    const char* name = PyUnicode_AsUTF8(item);
    if (name == nullptr || !parse_category_name(name, categories)) {
      Py_DECREF(item);
      Py_DECREF(iterator);
      return false;
    }
    Py_DECREF(item);
  }
  Py_DECREF(iterator);
  if (PyErr_Occurred()) {
    return false;
  }
  if (categories == archspec::CollectCategory::none) {
    PyErr_SetString(PyExc_ValueError, "categories must not be empty");
    return false;
  }
  return true;
}

bool assign_root(PyObject* value, const char* argument, std::string& destination) {
  if (value == nullptr || value == Py_None) {
    return true;
  }
  if (!PyUnicode_Check(value)) {
    PyErr_Format(PyExc_TypeError, "%s must be a string or None", argument);
    return false;
  }
  const char* text = PyUnicode_AsUTF8(value);
  if (text == nullptr) {
    return false;
  }
  destination = text;
  return true;
}

PyObject* collect_json(PyObject*, PyObject* args, PyObject* kwargs) {
  static const char* keywords[] = {
      "categories", "include_sensitive", "allow_perf_open", "allow_slow_probes",
      "allow_vendor_libraries", "procfs_root", "sysfs_root", "etc_root", "dev_root",
      nullptr,
  };
  PyObject* categories = Py_None;
  int include_sensitive = 0;
  int allow_perf_open = 0;
  int allow_slow_probes = 0;
  int allow_vendor_libraries = 0;
  PyObject* procfs_root = Py_None;
  PyObject* sysfs_root = Py_None;
  PyObject* etc_root = Py_None;
  PyObject* dev_root = Py_None;
  if (!PyArg_ParseTupleAndKeywords(
          args, kwargs, "|OppppOOOO:collect_json", const_cast<char**>(keywords),
          &categories, &include_sensitive, &allow_perf_open, &allow_slow_probes,
          &allow_vendor_libraries, &procfs_root, &sysfs_root, &etc_root, &dev_root)) {
    return nullptr;
  }

  archspec::CollectOptions options;
  if (!parse_categories(categories, options.categories) ||
      !assign_root(procfs_root, "procfs_root", options.procfs_root) ||
      !assign_root(sysfs_root, "sysfs_root", options.sysfs_root) ||
      !assign_root(etc_root, "etc_root", options.etc_root) ||
      !assign_root(dev_root, "dev_root", options.dev_root)) {
    return nullptr;
  }
  options.include_sensitive = include_sensitive != 0;
  options.allow_perf_open = allow_perf_open != 0;
  options.allow_slow_probes = allow_slow_probes != 0;
  options.allow_vendor_libraries = allow_vendor_libraries != 0;

  try {
    std::string result;
    Py_BEGIN_ALLOW_THREADS
    result = archspec::to_json(archspec::collect_report(options));
    Py_END_ALLOW_THREADS
    return PyUnicode_FromStringAndSize(result.data(), static_cast<Py_ssize_t>(result.size()));
  } catch (const std::exception& error) {
    PyErr_SetString(PyExc_RuntimeError, error.what());
  } catch (...) {
    PyErr_SetString(PyExc_RuntimeError, "archspec collection failed");
  }
  return nullptr;
}

PyObject* version(PyObject*, PyObject*) {
  return PyUnicode_FromString(archspec::version);
}

PyMethodDef kMethods[] = {
    {"collect_json", reinterpret_cast<PyCFunction>(collect_json), METH_VARARGS | METH_KEYWORDS,
     "Collect a report and return the native C++ JSON serialization."},
    {"version", version, METH_NOARGS, "Return the linked archspec library version."},
    {nullptr, nullptr, 0, nullptr},
};

PyModuleDef kModule = {
    PyModuleDef_HEAD_INIT,
    "_native",
    "Native archspec_inspect bindings.",
    -1,
    kMethods,
};

} // namespace

PyMODINIT_FUNC PyInit__native() {
  return PyModule_Create(&kModule);
}
