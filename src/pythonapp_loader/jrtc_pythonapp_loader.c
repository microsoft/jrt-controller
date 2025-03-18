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
do_stuff_in_thread(char* folder, char* python_script, PyInterpreterState* interp, void* args)
{
    printf("Running Python script: %s\n", python_script);
    fflush(stdout);

    // Step 1: Acquire the GIL
    PyGILState_STATE gstate = PyGILState_Ensure();

    // Step 2: Create new thread state for this interpreter
    printf("Creating new thread state...\n");
    fflush(stdout);
    PyThreadState* ts = PyThreadState_New(interp);
    if (!ts) {
        fprintf(stderr, "Error: Failed to create new thread state.\n");
        PyGILState_Release(gstate);
        return;
    }

    // Step 3: Swap to this thread state
    printf("Swapping to new thread state: %p\n", ts);
    fflush(stdout);
    PyThreadState_Swap(ts);

    // Run your Python code as usual here...
    PyObject* sysPath = PySys_GetObject("path");
    PyObject* temp = PyUnicode_FromString(folder);
    PyList_Append(sysPath, temp);
    Py_DECREF(temp);

    PyObject* pName = PyUnicode_DecodeFSDefault(python_script);
    PyObject* pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (!pModule) {
        fprintf(stderr, "Error: Failed to import module: %s.\n", python_script);
        PyErr_Print();
        goto cleanup;
    }

    PyObject* pFunc = PyObject_GetAttrString(pModule, "jrtc_start_app");
    if (pFunc && PyCallable_Check(pFunc)) {
        PyObject* pCapsule = PyCapsule_New(args, "void*", NULL);
        PyObject* pArgs = PyTuple_Pack(1, pCapsule);
        PyObject_CallObject(pFunc, pArgs);
        Py_XDECREF(pArgs);
        Py_XDECREF(pCapsule);
    } else {
        fprintf(stderr, "Error: Function 'jrtc_start_app' not callable.\n");
        PyErr_Print();
    }

    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);

cleanup:
    // Step 4: Proper cleanup
    PyThreadState_Clear(ts);
    PyThreadState_Swap(NULL);
    PyThreadState_Delete(ts);

    // Step 5: Release the GIL
    PyGILState_Release(gstate);
}

void*
run_subinterpreter(void* state)
{
    struct python_state* pstate = (struct python_state*)state;
    do_stuff_in_thread(pstate->folder, pstate->python_script, pstate->ts, pstate->args);
    return NULL;
}

void*
jrtc_start_app(void* args)
{
    struct jrtc_app_env* env_ctx = (struct jrtc_app_env*)args;
    char* full_path = env_ctx->params[0].val;
    char* folder = get_folder(full_path);
    char* python_script = get_file_name_without_py(full_path);

    if (!folder || !python_script) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        goto exit0;
    }

    if (!Py_IsInitialized()) {
        Py_Initialize();
    }

    // Don't release the GIL — Py_NewInterpreter handles it internally
    PyThreadState* main_ts = PyThreadState_Get();
    PyThreadState* ts1 = Py_NewInterpreter();
    if (!ts1) {
        fprintf(stderr, "Error: Failed to create new Python interpreter.\n");
        goto exit1;
    }

    // Swap to the new interpreter state to run Python code
    PyThreadState_Swap(ts1);

    if (!ts1) {
        fprintf(stderr, "Error: Failed to create new Python interpreter.\n");
        goto exit1;
    }

    struct python_state p1 = {folder, python_script, ts1->interp, args};
    run_subinterpreter(&p1);

    PyThreadState_Swap(ts1);
    Py_EndInterpreter(ts1);
    PyThreadState_Swap(main_ts);

exit1:
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
exit0:
    free(folder);
    free(python_script);
    return NULL;
}
