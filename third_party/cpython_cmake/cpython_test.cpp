// clang-format off
#include <Python.h>
// clang-format on

#include <assert.h>
#include <stdio.h>

#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
  #include <windows.h> 
#else
  #include <unistd.h> 
#endif

int main(int argc, char *argv[]) {
  printf("*********************************************************************\n");
  if (argc >= 2) {
    printf("************** This is to test CPython linking library **************\n");
    std::string env0 = std::string("PYTHONPATH=") + std::string(argv[1]) + "/lib/cpython";
    printf("%s\n", env0.c_str());
    putenv(const_cast<char*>(env0.c_str()));
    std::string env1 = std::string("PYTHONHOME=") + std::string(argv[1]) + "/bin";
    printf("%s\n", env1.c_str());
    putenv(const_cast<char*>(env1.c_str()));
    std::vector<std::string> commands = {
        "import os",    
        "import sys",
        "path = os.path.abspath(os.__file__)",
        "print('Hello World CPyton')",
        "print('OS Module Path: %s' % path)",
        "a = 1",
        "b = 2 + a",
        "x = 'I am CPython'",
        "y = x.lower()"};
    Py_Initialize();
    PyObject* dict = PyDict_New();
    PyObject* o = nullptr;
    for (auto& command : commands) {
      printf("CPython Command: %s\n", command.c_str());
      o = PyRun_String(command.c_str(), Py_single_input, dict, dict);
      if (o != nullptr) {
        Py_DECREF(o);
      }
    }
    // Check result
    // a
    PyObject* result = PyDict_GetItemString(dict, "a");
    assert(result != nullptr);
    assert(PyLong_Check(result));
    assert((uint32_t)(PyLong_AsLong(result)) == 1);
    // b
    result = PyDict_GetItemString(dict, "b");
    assert(result != nullptr);
    assert(PyLong_Check(result));
    assert((uint32_t)(PyLong_AsLong(result)) == 3);
    // x
    result = PyDict_GetItemString(dict, "x");
    assert(result != nullptr);
    assert(PyUnicode_Check(result));
    assert((std::string)(PyUnicode_AsUTF8(result)) == "I am CPython");
    // y
    result = PyDict_GetItemString(dict, "y");
    assert(result != nullptr);
    assert(PyUnicode_Check(result));
    assert((std::string)(PyUnicode_AsUTF8(result)) == "i am cpython");
    Py_DECREF(dict);
    Py_Finalize();
  } else {
    printf("**** Skip CPython Linking Testing because PATHS is not provided *****\n");
  }
  printf("*********************************************************************\n");
  return 0;
}