// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include "jrtc_router_app_api.h"
#include "jrtc.h"

/* Compiler magic to make address sanitizer ignore
memory leaks originating from libpython */
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_LEAK__)
__attribute__((used)) const char*
__asan_default_options()
{
    return "detect_odr_violation=0:intercept_tls_get_addr=0:suppression=libpython";
}

__attribute__((used)) const char*
__lsan_default_options()
{
    return "print_suppressions=0";
}

__attribute__((used)) const char*
__lsan_default_suppressions()
{
    return "leak:libpython";
}
#endif

char*
get_folder(const char* file_path)
{
    if (file_path == NULL) {
        return NULL; // Handle NULL input safely
    }

    char* folder = strdup(file_path);
    if (folder == NULL) {
        return NULL; // Memory allocation failed
    }

    char* last_slash = strrchr(folder, '/');
    if (last_slash != NULL) {
        // Edge case: If it's the only character (root `/`), return `/`
        if (last_slash == folder) {
            folder[1] = '\0';
        } else {
            *last_slash = '\0';
        }
    } else {
        // No `/` found, return "./"
        free(folder);
        return strdup("./");
    }

    return folder;
}

char*
get_file_name_without_py(const char* file_path)
{
    if (file_path == NULL) {
        return NULL; // Handle NULL input safely
    }

    // Find the last '/' in the file path
    const char* file_name = strrchr(file_path, '/');
    if (file_name != NULL) {
        file_name++; // Move past the '/'
    } else {
        file_name = file_path; // No '/' found, use the whole string
    }

    // Find the last '.' in the file name
    const char* last_dot = strrchr(file_name, '.');
    size_t name_length;
    if (last_dot != NULL && strcmp(last_dot, ".py") == 0) {
        name_length = last_dot - file_name; // Length without ".py"
    } else {
        name_length = strlen(file_name); // Full length
    }

    // Allocate memory for the new string
    char* result = (char*)malloc(name_length + 1);
    if (result == NULL) {
        return NULL; // Memory allocation failed
    }

    // Copy the relevant portion of the file name
    strncpy(result, file_name, name_length);
    result[name_length] = '\0'; // Null-terminate the string

    return result; // Caller must free the allocated memory
}

PyObject*
import_python_module(const char* python_path)
{
    PyObject* pModule = NULL;
    if (python_path == NULL) {
        fprintf(stderr, "Error: Python path is NULL.\n");
        return NULL;
    }

    // Extract the module name from the path
    char* module_name = get_file_name_without_py(python_path);
    if (module_name == NULL) {
        fprintf(stderr, "Error: Failed to extract module name from path: %s\n", python_path);
        return NULL;
    }

    char* path = get_folder(python_path);
    if (path == NULL) {
        fprintf(stderr, "Error: Failed to extract path from: %s\n", python_path);
        free(module_name);
        return NULL;
    }

    // Add the path to sys.path so Python can find the module
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject* sys_path = PySys_GetObject("path"); // Borrowed reference, no need to Py_DECREF
    PyObject* py_path = PyUnicode_DecodeFSDefault(path);
    if (sys_path && py_path) {
        if (PyList_Append(sys_path, py_path) < 0) {
            fprintf(stderr, "Error: Failed to append path to sys.path: %s\n", path);
        }
        Py_DECREF(py_path);
    } else {
        fprintf(stderr, "Error: Failed to access sys.path or create Python path object.\n");
        goto exit0;
    }

    // Import the module
    PyObject* pName = PyUnicode_DecodeFSDefault(module_name);
    if (!pName) {
        fprintf(stderr, "Error: Failed to create Python string for module name: %s\n", module_name);
        goto exit0;
    }

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (!pModule) {
        fprintf(stderr, "Error: Failed to import module: %s\n", module_name);
        PyErr_Print();
    }

exit0:
    PyGILState_Release(gstate);
    free(module_name);
    free(path);
    return pModule;
}

void*
jrtc_start_app(void* args)
{
    if (args == NULL) {
        fprintf(stderr, "Error: App context is NULL.\n");
        return NULL;
    }

    struct jrtc_app_env* env_ctx = args;
    char* full_path = env_ctx->params[0].val;

    // Initialize the Python interpreter (only once)
    if (!Py_IsInitialized()) {
        Py_Initialize();
        printf("Python interpreter initialized.\n");
    }

    // Acquire GIL (if multi-threaded)
    PyGILState_STATE gstate = PyGILState_Ensure();

    // Create a Python capsule for the `args`
    PyObject* pCapsule = PyCapsule_New(args, "void*", NULL);
    if (!pCapsule) {
        fprintf(stderr, "Error: Failed to create Python capsule.\n");
        goto cleanup_gil;
    }

    // Load additional modules
    for (int i = 0; i < MAX_APP_MODULES; i++) {
        if (env_ctx->app_modules[i] == NULL) {
            break;
        }
        printf("Loading Module: %s\n", env_ctx->app_modules[i]);
        PyObject* module = import_python_module(env_ctx->app_modules[i]);
        if (!module) {
            fprintf(stderr, "Error: Failed to import module: %s.\n", env_ctx->app_modules[i]);
            goto cleanup_capsule;
        }
        printf("Module loaded: %s\n", env_ctx->app_modules[i]);
        PyObject* sysModule = PyImport_ImportModule("sys");
        PyObject* sysDict = PyModule_GetDict(sysModule);
        PyObject* modules = PyDict_GetItemString(sysDict, "modules");
        // Inject
        char* module_name = get_file_name_without_py(env_ctx->app_modules[i]);
        PyDict_SetItemString(modules, module_name, module);
        free(module_name);
        Py_DECREF(sysModule);
        Py_DECREF(module);
    }

    // Import the main Python module
    PyObject* pModule = import_python_module(full_path);
    if (!pModule) {
        fprintf(stderr, "Error: Failed to import main module: %s.\n", full_path);
        goto cleanup_capsule;
    }

    // Get the Python function
    PyObject* pFunc = PyObject_GetAttrString(pModule, "jrtc_start_app");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        fprintf(stderr, "Error: Function 'jrtc_start_app' is not callable.\n");
        PyErr_Print();
        goto cleanup_module;
    }

    // Call the Python function with the capsule
    PyObject* pArgs = PyTuple_Pack(1, pCapsule);
    if (!pArgs) {
        fprintf(stderr, "Error: Failed to create arguments tuple.\n");
        PyErr_Print();
        goto cleanup_func;
    }

    PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);

    if (!pResult) {
        fprintf(stderr, "Error: Function call failed.\n");
        PyErr_Print();
    } else {
        Py_DECREF(pResult);
    }

cleanup_func:
    Py_XDECREF(pFunc);
cleanup_module:
    Py_XDECREF(pModule);
cleanup_capsule:
    Py_XDECREF(pCapsule);
cleanup_gil:
    PyGILState_Release(gstate);

    if (Py_IsInitialized()) {
        Py_Finalize();
        printf("Python interpreter finalized.\n");
    }
    return NULL;
}
