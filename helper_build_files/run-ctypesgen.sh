#!/bin/bash
unset ASAN_OPTIONS
unset LD_PRELOAD
exec ctypesgen "$@"