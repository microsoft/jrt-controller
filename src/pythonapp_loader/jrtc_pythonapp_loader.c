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

#define PYTHON_ENTRYPOINT "jrtc_start_app"

struct python_state
{
    char* full_python_path;
    PyInterpreterState* ts;
    void* args;
};

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
        return strdup("./");
    }

    char* folder = strdup(file_path);
    if (folder == NULL) {
        return NULL;
    }

    char* last_slash = strrchr(folder, '/');
    if (last_slash != NULL) {
        if (last_slash == folder) {
            folder[1] = '\0';
        } else {
            *last_slash = '\0';
        }
    } else {
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
    // PyGILState_STATE gstate = PyGILState_Ensure();
    // PyThreadState* tstate = PyThreadState_Get();
    // PyEval_RestoreThread(tstate);
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
    // PyGILState_Release(gstate);
    // PyEval_SaveThread();
    free(module_name);
    free(path);
    return pModule;
}

void
run_python_using_interpreter(char* python_script, PyInterpreterState* interp, void* args)
{
    printf("Running Python script: %s\n", python_script);
    fflush(stdout);

    if (args == NULL) {
        fprintf(stderr, "Error: Received NULL argument for PyCapsule_New\n");
        return;
    }

    // Create sub-interpreter only if needed
    PyThreadState* ts = NULL;
    if (interp) {
        ts = PyThreadState_New(interp);
        if (!ts) {
            fprintf(stderr, "Error: Failed to create new thread state.\n");
            return;
        }
    }

    PyObject* pModule = NULL;
    PyObject* pFunc = NULL;
    PyObject* pArgs = NULL;
    PyObject* pCapsule = NULL;

    // Ensure we are using the correct thread state
    PyThreadState* mainThreadState = PyThreadState_Get(); // Save the main thread state
    if (ts) {
        // Swap to sub-interpreter's thread state
        PyThreadState_Swap(ts);
    }

    pModule = import_python_module(python_script);
    if (!pModule) {
        fprintf(stderr, "Error: Failed to import module: %s.\n", python_script);
        PyErr_Print();
        goto cleanup;
    }

    pFunc = PyObject_GetAttrString(pModule, PYTHON_ENTRYPOINT);
    if (!pFunc || !PyCallable_Check(pFunc)) {
        fprintf(stderr, "Error: Cannot find function '%s' in module '%s'.\n", PYTHON_ENTRYPOINT, python_script);
        PyErr_Print();
        goto cleanup;
    }

    pCapsule = PyCapsule_New(args, "void*", NULL);
    if (!pCapsule) {
        fprintf(stderr, "Error: PyCapsule_New failed.\n");
        goto cleanup;
    }

    pArgs = PyTuple_Pack(1, pCapsule);
    if (!pArgs) {
        fprintf(stderr, "Error: PyTuple_Pack failed.\n");
        Py_XDECREF(pCapsule);
        goto cleanup;
    }

    PyObject_CallObject(pFunc, pArgs);

cleanup:
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);
    Py_XDECREF(pArgs);
    Py_XDECREF(pCapsule);

    if (PyErr_Occurred()) {
        PyErr_Print();
        fprintf(stderr, "Error: Exception occurred while running Python script.\n");
    }

    // Clean up: Swap back to the main interpreter's thread state
    if (ts) {
        PyThreadState_Swap(mainThreadState);
        PyThreadState_Clear(ts);
        PyThreadState_Delete(ts);
    }
}

void*
run_subinterpreter(void* state)
{
    struct python_state* pstate = (struct python_state*)state;
    run_python_using_interpreter(pstate->full_python_path, pstate->ts, pstate->args);
    return NULL;
}

void*
jrtc_start_app(void* args)
{
    if (args == NULL) {
        fprintf(stderr, "Error: App context is NULL.\n");
        return NULL;
    }

    struct jrtc_app_env* env_ctx = (struct jrtc_app_env*)args;
    char* full_path = env_ctx->params[0].val;
    printf("Full path: %s\n", full_path);
    char* python_type = env_ctx->params[0].key;
    printf("Python type: %s\n", python_type);

    // Initialize the Python interpreter (only once)
    if (!Py_IsInitialized()) {
        Py_Initialize();
        printf("Python interpreter initialized.\n");
    }

    // Acquire GIL (if multi-threaded)
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyThreadState* ts1 = NULL;

    // Create a Python capsule for the `args`
    PyObject* pCapsule = PyCapsule_New(args, "void*", NULL);
    if (!pCapsule) {
        fprintf(stderr, "Error: Failed to create Python capsule.\n");
        goto cleanup_gil;
    }

    // === Load additional modules in the main interpreter ===
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
        Py_DECREF(module);
        printf("Module loaded: %s\n", env_ctx->app_modules[i]);
    }

    // === Create and switch to sub-interpreter ===
    PyThreadState* main_ts = PyThreadState_Get();
    ts1 = Py_NewInterpreter(); // Create a sub-interpreter
    if (!ts1) {
        fprintf(stderr, "Error: Failed to create sub-interpreter.\n");
        goto cleanup_capsule;
    }

    // Inject modules directly into the subinterpreter's sys.modules
    PyObject* sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        fprintf(stderr, "Error: Failed to import 'sys' in sub-interpreter.\n");
        goto cleanup_capsule;
    }

    PyObject* sysDict = PyModule_GetDict(sysModule);
    PyObject* modules = PyDict_GetItemString(sysDict, "modules");

    if (!modules) {
        fprintf(stderr, "Error: Failed to get 'sys.modules' in sub-interpreter.\n");
        Py_DECREF(sysModule);
        goto cleanup_capsule;
    }

    for (int i = 0; i < MAX_APP_MODULES; i++) {
        if (env_ctx->app_modules[i] == NULL) {
            break;
        }

        char* module_name = get_file_name_without_py(env_ctx->app_modules[i]);
        printf("Injecting Module: %s\n", module_name);
        PyObject* module = import_python_module(env_ctx->app_modules[i]);

        if (!module) {
            fprintf(stderr, "Error: Failed to import module '%s' in sub-interpreter.\n", env_ctx->app_modules[i]);
            free(module_name);
            continue;
        }

        // Inject module into sub-interpreter's sys.modules
        PyDict_SetItemString(modules, module_name, module);
        printf("Module injected into sub-interpreter: %s\n", module_name);

        Py_DECREF(module);
        free(module_name);
    }

    Py_DECREF(sysModule);

    // === Execute in Sub-interpreter ===
    struct python_state pstate = {.full_python_path = full_path, .ts = ts1->interp, .args = args};
    run_subinterpreter(&pstate);

cleanup_capsule:
    Py_XDECREF(pCapsule);

cleanup_gil:

    if (ts1) {
        // === Clean up Sub-interpreter ===
        PyThreadState_Swap(ts1);
        Py_EndInterpreter(ts1);
        PyThreadState_Swap(main_ts);
    }
    PyGILState_Release(gstate);

    if (Py_IsInitialized()) {
        Py_Finalize();
        printf("Python interpreter finalized.\n");
    } else {
        fprintf(stderr, "Error: Python interpreter was not initialized.\n");
    }

    return NULL;
}
