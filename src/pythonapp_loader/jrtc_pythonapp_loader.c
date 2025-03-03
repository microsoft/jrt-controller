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

void*
jrtc_start_app(void* args)
{
    if (args == NULL) {
        fprintf(stderr, "Error: App context is NULL.\n");
        return NULL;
    }
    struct jrtc_app_env* env_ctx = args;
    char* full_path = env_ctx->app_path;
    printf("Python Full Path: %s\n", full_path);
    // extract the folder of `full_path`
    char* folder = get_folder(full_path);
    printf("Folder: %s\n", folder);
    // python_script should be the filename without the .py
    char* python_script = get_file_name_without_py(full_path);
    printf("Python Script: %s\n", python_script);

    // Initialize the Python interpreter
    if (!Py_IsInitialized()) {
        Py_Initialize();
        printf("Python interpreter initialized.\n");
    }

    // Create a Python capsule for the `args`
    PyObject* pCapsule = PyCapsule_New(args, "void*", NULL);
    if (!pCapsule) {
        fprintf(stderr, "Error: Failed to create Python capsule.\n");
        goto exit2;
    }

    // Add current working directory to the Python path
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "Error: Failed to get current working directory.\n");
        goto exit1;
    }
    printf("Current working directory: %s\n", cwd);

    PyObject* sysPath = PySys_GetObject("path");
    if (!sysPath) {
        fprintf(stderr, "Error: Failed to get Python sys.path.\n");
        goto exit1;
    }

    PyObject* path = PyUnicode_FromString(cwd);
    if (!path) {
        fprintf(stderr, "Error: Failed to create Python string for cwd.\n");
        goto exit1;
    }

    PyList_Append(sysPath, path);
    PyList_Append(sysPath, PyUnicode_FromString(folder));
    Py_DECREF(path);

    // Import the Python module
    PyObject* pName = PyUnicode_DecodeFSDefault(python_script);
    if (!pName) {
        fprintf(stderr, "Error: Failed to create Python string for module name %s.\n", python_script);
        goto exit1;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject* pModule = PyImport_Import(pName);
    PyGILState_Release(gstate);

    PyErr_Print();

    printf("Importing Python Module: %s\n", python_script);
    Py_DECREF(pName);

    if (!pModule) {
        fprintf(stderr, "Error: Failed to import module: %s.\n", python_script);
        PyErr_Print();
        goto exit1;
    }

    // Get the Python function
    PyObject* pFunc = PyObject_GetAttrString(pModule, "jrtc_start_app");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        fprintf(stderr, "Error: Function 'jrtc_start_app' is not callable.\n");
        PyErr_Print();
        goto exit0;
    }

    // Prepare arguments and call the Python function
    PyObject* pArgs = PyTuple_Pack(1, pCapsule);
    if (!pArgs) {
        fprintf(stderr, "Error: Failed to create arguments tuple.\n");
        PyErr_Print();
        goto exit0;
    }

    PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);

    if (!pResult) {
        fprintf(stderr, "Error: Function call failed.\n");
        PyErr_Print();
        goto exit0;
    } else {
        Py_DECREF(pResult);
    }

    // Clean up
exit0:
    Py_DECREF(pFunc);
    Py_DECREF(pModule);
exit1:
    Py_XDECREF(pCapsule);
exit2:
    // Finalize the Python interpreter
    if (Py_IsInitialized()) {
        Py_Finalize();
    }

    // Free the allocated memory
    free(folder);
    free(python_script);
    return NULL;
}