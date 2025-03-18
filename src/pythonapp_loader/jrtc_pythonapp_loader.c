// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include "jrtc_router_app_api.h"
#include "jrtc.h"

#define PYTHON_ENTRYPOINT "jrtc_start_app"

struct python_state
{
    char* folder;
    char* python_script;
    PyInterpreterState* ts;
    void* args;
};

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
        return NULL;
    }

    const char* file_name = strrchr(file_path, '/');
    file_name = (file_name != NULL) ? file_name + 1 : file_path;

    const char* last_dot = strrchr(file_name, '.');
    size_t name_length =
        (last_dot && strcmp(last_dot, ".py") == 0) ? (size_t)(last_dot - file_name) : strlen(file_name);

    char* result = (char*)malloc(name_length + 1);
    if (result == NULL) {
        return NULL;
    }

    strncpy(result, file_name, name_length);
    result[name_length] = '\0';

    return result;
}

void
run_python_without_interpreter(char* folder, char* python_script, void* args)
{
    printf("Running Python script (Single App): %s\n", python_script);
    fflush(stdout);

    // Acquire GIL just once when interacting with Python
    PyGILState_STATE gstate = PyGILState_Ensure();

    if (args == NULL) {
        fprintf(stderr, "Error: Received NULL argument for PyCapsule_New\n");
        PyGILState_Release(gstate);
        return;
    }

    // Add folder to sys.path
    PyObject* sysPath = PySys_GetObject("path");
    PyObject* temp = PyUnicode_FromString(folder);
    PyList_Append(sysPath, temp);
    Py_DECREF(temp);

    // Import the Python script/module
    PyObject* pName = PyUnicode_DecodeFSDefault(python_script);
    PyObject* pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (!pModule) {
        fprintf(stderr, "Error: Failed to import module: %s.\n", python_script);
        PyErr_Print();
        PyGILState_Release(gstate);
        return;
    }

    // Get the function reference
    PyObject* pFunc = PyObject_GetAttrString(pModule, PYTHON_ENTRYPOINT);
    if (!pFunc || !PyCallable_Check(pFunc)) {
        fprintf(stderr, "Error: Cannot find function '%s' in module '%s'.\n", PYTHON_ENTRYPOINT, python_script);
        PyErr_Print();
        Py_XDECREF(pModule);
        PyGILState_Release(gstate);
        return;
    }

    // Pass the arguments to the Python function
    PyObject* pCapsule = PyCapsule_New(args, "void*", NULL);
    if (!pCapsule) {
        fprintf(stderr, "Error: PyCapsule_New failed.\n");
        Py_XDECREF(pModule);
        PyGILState_Release(gstate);
        return;
    }

    PyObject* pArgs = PyTuple_Pack(1, pCapsule);
    if (!pArgs) {
        fprintf(stderr, "Error: PyTuple_Pack failed.\n");
        Py_XDECREF(pCapsule);
        Py_XDECREF(pModule);
        PyGILState_Release(gstate);
        return;
    }

    // Call the function
    PyObject_CallObject(pFunc, pArgs);
    Py_XDECREF(pArgs);
    Py_XDECREF(pCapsule);
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);

    // Release the GIL
    PyGILState_Release(gstate);
}

void
run_python_using_interpreter(char* folder, char* python_script, PyInterpreterState* interp, void* args)
{
    printf("Running Python script (Multiple Apps): %s\n", python_script);
    fflush(stdout);

    // Acquire GIL just once when interacting with Python
    PyGILState_STATE gstate = PyGILState_Ensure();

    if (args == NULL) {
        fprintf(stderr, "Error: Received NULL argument for PyCapsule_New\n");
        PyGILState_Release(gstate);
        return;
    }

    // We use the provided interpreter state for single-threaded mode, no additional thread state needed.
    PyThreadState* ts = PyThreadState_New(interp);
    if (!ts) {
        fprintf(stderr, "Error: Failed to create new thread state.\n");
        PyGILState_Release(gstate);
        return;
    }

    PyThreadState_Swap(ts);

    // Add folder to sys.path
    PyObject* sysPath = PySys_GetObject("path");
    PyObject* temp = PyUnicode_FromString(folder);
    PyList_Append(sysPath, temp);
    Py_DECREF(temp);

    // Import the Python script/module
    PyObject* pName = PyUnicode_DecodeFSDefault(python_script);
    PyObject* pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (!pModule) {
        fprintf(stderr, "Error: Failed to import module: %s.\n", python_script);
        PyErr_Print();
        PyGILState_Release(gstate);
        PyThreadState_Swap(NULL);
        PyThreadState_Clear(ts);
        PyThreadState_Delete(ts);
        return;
    }

    // Get the function reference
    PyObject* pFunc = PyObject_GetAttrString(pModule, PYTHON_ENTRYPOINT);
    if (!pFunc || !PyCallable_Check(pFunc)) {
        fprintf(stderr, "Error: Cannot find function '%s' in module '%s'.\n", PYTHON_ENTRYPOINT, python_script);
        PyErr_Print();
        Py_XDECREF(pModule);
        PyGILState_Release(gstate);
        PyThreadState_Swap(NULL);
        PyThreadState_Clear(ts);
        PyThreadState_Delete(ts);
        return;
    }

    PyObject* pCapsule = PyCapsule_New(args, "void*", NULL);
    if (!pCapsule) {
        fprintf(stderr, "Error: PyCapsule_New failed.\n");
        Py_XDECREF(pModule);
        PyGILState_Release(gstate);
        PyThreadState_Swap(NULL);
        PyThreadState_Clear(ts);
        PyThreadState_Delete(ts);
        return;
    }

    PyObject* pArgs = PyTuple_Pack(1, pCapsule);
    if (!pArgs) {
        fprintf(stderr, "Error: PyTuple_Pack failed.\n");
        Py_XDECREF(pCapsule);
        Py_XDECREF(pModule);
        PyGILState_Release(gstate);
        PyThreadState_Swap(NULL);
        PyThreadState_Clear(ts);
        PyThreadState_Delete(ts);
        return;
    }

    // Call the function
    PyObject_CallObject(pFunc, pArgs);
    Py_XDECREF(pArgs);
    Py_XDECREF(pCapsule);
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);

    // Clean up thread state
    PyThreadState_Swap(NULL);
    PyThreadState_Clear(ts);
    PyThreadState_Delete(ts);

    // Release GIL
    PyGILState_Release(gstate);
}

void*
run_subinterpreter(void* state)
{
    struct python_state* pstate = (struct python_state*)state;
    run_python_using_interpreter(pstate->folder, pstate->python_script, pstate->ts, pstate->args);
    return NULL;
}

void*
jrtc_start_app(void* args)
{
    struct jrtc_app_env* env_ctx = (struct jrtc_app_env*)args;
    char* full_path = env_ctx->params[0].val;
    char* python_type = env_ctx->params[0].key;
    printf("Python type: %s\n", python_type);
    fflush(stdout);
    char* folder = get_folder(full_path);
    char* python_script = get_file_name_without_py(full_path);

    if (!folder || !python_script) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        goto exit0;
    }

    if (!Py_IsInitialized()) {
        Py_Initialize();
    }

    if (strcmp(python_type, "python_single_app") == 0) {
        run_python_without_interpreter(folder, python_script, args);
        goto exit1;
    }

    // Initialize the main interpreter and create sub-interpreter
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyThreadState* main_ts = PyThreadState_Get();
    PyThreadState* ts1 = Py_NewInterpreter(); // Create a sub-interpreter
    if (!ts1) {
        fprintf(stderr, "Error: Failed to create new Python interpreter.\n");
        goto exit2;
    }

    // Swap to the new interpreter state for execution
    PyThreadState_Swap(ts1);

    struct python_state p1 = {folder, python_script, ts1->interp, args};
    run_subinterpreter(&p1);

    // Clean up the sub-interpreter state
    PyThreadState_Swap(ts1);
    Py_EndInterpreter(ts1);

    PyThreadState_Swap(main_ts);

exit2:
    PyGILState_Release(gstate);

exit1:
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
exit0:
    free(folder);
    free(python_script);
    return NULL;
}
