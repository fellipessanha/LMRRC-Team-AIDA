// https://www.codeproject.com/articles/820116/embedding-python-program-in-a-c-cplusplus-code
//
#include "pyhelper.hpp"
#include <Python.h>
#include <optional>
#include <stdio.h>

template<class T>
using op = std::optional<T>;

op<int>
call_loadRouteId(std::string param)
{
   std::cout << "will load module 'pyInvokeEvaluator'" << std::endl;
   std::cout << "OBS: this module should likely be on './build_py/pyInvokeEvaluator.py'" << std::endl;
   CPyObject pName = PyUnicode_FromString("pyInvokeEvaluator");
   CPyObject pModule = PyImport_Import(pName);
   if (!pModule)
      return std::nullopt;

   CPyObject loadRouteId = PyObject_GetAttrString(pModule, "loadRouteId");

   if (!loadRouteId || !PyCallable_Check(loadRouteId))
      return std::nullopt;

   PyObject* kwargs = Py_BuildValue("(s)", param.c_str());

   CPyObject pValue = PyObject_CallObject(loadRouteId, kwargs);
   long ret_l = PyLong_AsLong(pValue);
   printf("C: loadRouteId() = %ld\n", PyLong_AsLong(pValue));
   //
   //delete kwargs;
   //
   return ret_l;
}

int
main()
{
   CPyInstance hInstance;

   PyRun_SimpleString("print('Hello World from Embedded Python!!!')");

   std::string param = "{\"A\":10}";
   int r = *call_loadRouteId(param);
   std::cout << "r=" << r << std::endl;

   std::cout << "FINISHED SUCCESSFULLY" << std::endl;
   return 0;
}