// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include <pthread.h>
#include <stdatomic.h>
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include "jrtc_router_app_api.h"
#include "jrtc_shared_python_state.h"
#include "jrtc.h"

#define PYTHON_ENTRYPOINT "jrtc_start_app"

struct python_state
{
    char* full_python_path;
    PyInterpreterState* ts;
    void* args;
};

void
printf_and_flush(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}

void
fprintf_and_flush(FILE* stream, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
    fflush(stream);
}

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
        fprintf_and_flush(stderr, "Error: Python path is NULL.\n");
        return NULL;
    }

    char* module_name = NULL;
    char* path = NULL;

    if (python_path == NULL || strlen(python_path) == 0) {
        fprintf_and_flush(stderr, "Error: Invalid Python path provided.\n");
        return NULL;
    }
    module_name = get_file_name_without_py(python_path);
    if (module_name == NULL) {
        fprintf_and_flush(stderr, "Error: Failed to extract module name from path: %s\n", python_path);
        return NULL;
    }

    path = get_folder(python_path);
    if (path == NULL) {
        fprintf_and_flush(stderr, "Error: Failed to extract path from: %s\n", python_path);
        free(module_name);
        return NULL;
    }

    PyObject* sys_path = PySys_GetObject("path"); // Borrowed reference, no need to Py_DECREF
    PyObject* py_path = PyUnicode_DecodeFSDefault(path);
    if (sys_path && py_path) {
        if (PyList_Append(sys_path, py_path) < 0) {
            fprintf_and_flush(stderr, "Error: Failed to append path to sys.path: %s\n", path);
        }
        Py_DECREF(py_path);
    } else {
        fprintf_and_flush(stderr, "Error: Failed to access sys.path or create Python path object.\n");
        goto exit0;
    }

    // Import the module
    PyObject* pName = PyUnicode_DecodeFSDefault(module_name);
    if (!pName) {
        fprintf_and_flush(stderr, "Error: Failed to create Python string for module name: %s\n", module_name);
        goto exit0;
    }

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (!pModule) {
        fprintf_and_flush(stderr, "Error: Failed to import module: %s\n", module_name);
        PyErr_Print();
    }

exit0:
    free(module_name);
    free(path);
    return pModule;
}

void
run_python_using_interpreter(char* python_script, PyInterpreterState* interp, void* args)
{
    printf_and_flush("Running Python script: %s\n", python_script);

    if (args == NULL) {
        fprintf_and_flush(stderr, "Error: Received NULL argument for PyCapsule_New\n");
        return;
    }

    // Create sub-interpreter only if needed
    PyThreadState* ts = NULL;
    if (interp) {
        ts = PyThreadState_New(interp);
        if (!ts) {
            fprintf_and_flush(stderr, "Error: Failed to create new thread state.\n");
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
        fprintf_and_flush(stderr, "Error: Failed to import module: %s.\n", python_script);
        PyErr_Print();
        goto cleanup;
    }

    pFunc = PyObject_GetAttrString(pModule, PYTHON_ENTRYPOINT);
    if (!pFunc || !PyCallable_Check(pFunc)) {
        fprintf_and_flush(
            stderr, "Error: Cannot find function '%s' in module '%s'.\n", PYTHON_ENTRYPOINT, python_script);
        PyErr_Print();
        goto cleanup;
    }

    pCapsule = PyCapsule_New(args, "void*", NULL);
    if (!pCapsule) {
        fprintf_and_flush(stderr, "Error: PyCapsule_New failed.\n");
        goto cleanup;
    }

    pArgs = PyTuple_Pack(1, pCapsule);
    if (!pArgs) {
        fprintf_and_flush(stderr, "Error: PyTuple_Pack failed.\n");
        Py_XDECREF(pCapsule);
        goto cleanup;
    }

    printf_and_flush("Calling Python function %s...\n", python_script);
    PyObject_CallObject(pFunc, pArgs);
    if (PyErr_Occurred()) {
        PyErr_Print();
        fprintf_and_flush(stderr, "Error: Exception occurred while calling Python function.\n");
        goto cleanup;
    }

cleanup:
    printf_and_flush("Cleaning up Python script execution %s...\n", python_script);
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);
    Py_XDECREF(pArgs);
    Py_XDECREF(pCapsule);

    if (PyErr_Occurred()) {
        PyErr_Print();
        fprintf_and_flush(stderr, "Error: Exception occurred while running Python script.\n");
    }

    // Clean up: Swap back to the main interpreter's thread state
    if (ts) {
        printf_and_flush("Cleaning up sub-interpreter thread state %s...\n", python_script);
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
    if (PyErr_Occurred()) {
        PyErr_Print();
        fprintf_and_flush(stderr, "Error: Exception occurred in sub-interpreter.\n");
    }
    return NULL;
}

void*
jrtc_start_app(void* args)
{
    if (args == NULL) {
        fprintf_and_flush(stderr, "Error: App context is NULL.\n");
        return NULL;
    }

    struct jrtc_app_env* env_ctx = (struct jrtc_app_env*)args;
    shared_python_state_t* shared_python_state = env_ctx->shared_python_state;
    if (shared_python_state == NULL) {
        fprintf_and_flush(stderr, "Error: Shared Python state is NULL.\n");
        return NULL;
    }

    char* full_path = env_ctx->params[0].val;
    printf_and_flush("Python App Full path: %s\n", full_path);

    PyThreadState* my_sub_ts = NULL;

    // Initialize Python interpreter once and create a new sub-interpreter per thread
    pthread_mutex_lock(&shared_python_state->python_lock);
    if (!shared_python_state->initialized) {
        if (!Py_IsInitialized()) {
            Py_Initialize();
            shared_python_state->main_ts = PyThreadState_Get();
        }
        shared_python_state->initialized = true;
        printf_and_flush("Python interpreter initialized.\n");
    }

    // Create sub-interpreter for this thread
    my_sub_ts = Py_NewInterpreter();
    if (!my_sub_ts) {
        fprintf(stderr, "Failed to create sub-interpreter\n");
        pthread_mutex_unlock(&shared_python_state->python_lock);
        return NULL;
    }

    // Release the thread state so other threads can create theirs
    PyEval_ReleaseThread(my_sub_ts);
    pthread_mutex_unlock(&shared_python_state->python_lock);

    printf_and_flush("Starting Python app: %s\n", full_path);

    // Acquire the GIL and this thread's sub-interpreter thread state
    PyEval_AcquireThread(my_sub_ts);

    // Create Python capsule for args
    PyObject* pCapsule = PyCapsule_New(args, "void*", NULL);
    if (!pCapsule) {
        fprintf_and_flush(stderr, "Error: Failed to create Python capsule.\n");
        PyErr_Print();
        goto cleanup_thread;
    }

    // Load modules in the main interpreter if needed (optional, adjust per your logic)
    for (int i = 0; i < MAX_APP_MODULES; i++) {
        if (env_ctx->app_modules[i] == NULL) {
            break;
        }
        printf_and_flush("Loading Module: %s\n", env_ctx->app_modules[i]);
        PyObject* module = import_python_module(env_ctx->app_modules[i]);
        if (!module) {
            fprintf_and_flush(stderr, "Error: Failed to import module: %s.\n", env_ctx->app_modules[i]);
            goto cleanup_capsule;
        }
        Py_DECREF(module);
        printf_and_flush("Module loaded: %s\n", env_ctx->app_modules[i]);
    }

    // Inject modules into sub-interpreter's sys.modules
    PyObject* sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        fprintf_and_flush(stderr, "Error: Failed to import 'sys' in sub-interpreter.\n");
        goto cleanup_capsule;
    }

    PyObject* sysDict = PyModule_GetDict(sysModule);
    PyObject* modules = PyDict_GetItemString(sysDict, "modules");
    if (!modules) {
        fprintf_and_flush(stderr, "Error: Failed to get 'sys.modules' in sub-interpreter.\n");
        Py_DECREF(sysModule);
        goto cleanup_capsule;
    }

    for (int i = 0; i < MAX_APP_MODULES; i++) {
        if (env_ctx->app_modules[i] == NULL) {
            break;
        }

        char* module_name = get_file_name_without_py(env_ctx->app_modules[i]);
        printf_and_flush("Injecting Module: %s\n", module_name);
        PyObject* module = import_python_module(env_ctx->app_modules[i]);
        if (!module) {
            fprintf_and_flush(
                stderr, "Error: Failed to import module '%s' in sub-interpreter.\n", env_ctx->app_modules[i]);
            free(module_name);
            continue;
        }

        if (PyDict_SetItemString(modules, module_name, module) < 0) {
            PyErr_Print();
            fprintf_and_flush(stderr, "Failed to inject module: %s\n", module_name);
        } else {
            printf_and_flush("Module injected into sub-interpreter: %s\n", module_name);
        }

        Py_DECREF(module);
        free(module_name);
    }
    Py_DECREF(sysModule);

    // Run your sub-interpreter Python code here
    struct python_state pstate = {.full_python_path = full_path, .ts = my_sub_ts->interp, .args = args};
    run_subinterpreter(&pstate);

cleanup_capsule:
    Py_XDECREF(pCapsule);

cleanup_thread:
    printf_and_flush("Cleaning up sub-interpreter: %s...\n", full_path);

    // Release GIL and thread state after run_subinterpreter
    PyEval_ReleaseThread(my_sub_ts);

    // Now reacquire thread state so it's current for Py_EndInterpreter
    pthread_mutex_lock(&shared_python_state->python_lock);
    PyEval_AcquireThread(my_sub_ts);
    Py_EndInterpreter(my_sub_ts);
    pthread_mutex_unlock(&shared_python_state->python_lock);

    printf_and_flush("Skipping Python interpreter finalization for %s\n", full_path);
    printf_and_flush("Python interpreter finalized.\n");
    printf_and_flush("Python app terminated: %s\n", full_path);

    return NULL;
}
