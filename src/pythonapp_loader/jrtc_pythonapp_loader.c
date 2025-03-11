// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include <Python.h>
#include <pthread.h>
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

void do_stuff_in_thread(char* folder, char* python_script, PyInterpreterState* interp, void* args)
{
    printf("Running Python script: %s\n", python_script);
    fflush(stdout);

    // Debug: Print interpreter state
    printf("Python Interpreter State: %p\n", interp);
    fflush(stdout);
    if (!interp) {
        fprintf(stderr, "Error: Invalid interpreter state.\n");
        return;
    }

    // Create new thread state
    printf("Creating new thread state...\n");
    fflush(stdout);
    PyThreadState* ts = PyThreadState_New(interp);
    if (!ts) {
        fprintf(stderr, "Error: Failed to create new thread state.\n");
        return;
    }

    // Properly swap to the new thread state (instead of PyEval_RestoreThread)
    printf("Swapping to new thread state: %p\n", ts);
    fflush(stdout);
    PyThreadState_Swap(ts);  // Swaps current state to `ts` (acquiring the subinterpreter's GIL)
    
    PyObject* pCapsule = PyCapsule_New(args, "void*", NULL);
    if (!pCapsule) {
        fprintf(stderr, "Error: Failed to create Python capsule.\n");
        goto cleanup;
    }

    // Append folder to sys.path safely
    PyObject* sysPath = PySys_GetObject("path");
    if (sysPath) {
        PyObject* temp = PyUnicode_FromString(folder);
        if (temp) {
            PyList_Append(sysPath, temp);
            Py_DECREF(temp);
        }
    }

    // Import the Python script
    PyObject* pName = PyUnicode_DecodeFSDefault(python_script);
    if (!pName) {
        fprintf(stderr, "Error: Failed to create Python string for module name.\n");
        goto cleanup;
    }

    PyObject* pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (!pModule) {
        fprintf(stderr, "Error: Failed to import module: %s.\n", python_script);
        PyErr_Print();
        goto cleanup;
    }

    // Get function
    PyObject* pFunc = PyObject_GetAttrString(pModule, "jrtc_start_app");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        fprintf(stderr, "Error: Function 'jrtc_start_app' is not callable.\n");
        PyErr_Print();
        goto cleanup_module;
    }

    // Call the function
    PyObject* pArgs = PyTuple_Pack(1, pCapsule);
    if (pArgs) {
        PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
        Py_DECREF(pArgs);

        if (!pResult) {
            fprintf(stderr, "Error: Function call failed.\n");
            PyErr_Print();
        } else {
            Py_DECREF(pResult);
        }
    }

cleanup_module:
    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);
cleanup:
    Py_XDECREF(pCapsule);

    // Properly release the thread state
    PyThreadState_Clear(ts);
    PyThreadState_Swap(NULL);  // Unset the current thread state
    PyThreadState_Delete(ts);
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
    char* full_path = env_ctx->app_params[0];
    char* folder = get_folder(full_path);
    char* python_script = get_file_name_without_py(full_path);

    if (!folder || !python_script) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        goto exit0;
    }

    if (!Py_IsInitialized()) {
        Py_Initialize();
    } else {
        printf("Releasing GIL before creating a new interpreter...\n");
        PyEval_SaveThread();  // Releases GIL
    }

    PyThreadState* main_ts = PyThreadState_Get();
    PyThreadState* ts1 = Py_NewInterpreter();
    if (!ts1) {
        fprintf(stderr, "Error: Failed to create new Python interpreter.\n");
        goto exit1;
    }

    struct python_state p1 = {folder, python_script, ts1->interp, args};
    pthread_t thread1;
    pthread_create(&thread1, NULL, run_subinterpreter, (void*)&p1);
    pthread_join(thread1, NULL);

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
