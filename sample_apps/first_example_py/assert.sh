#!/bin/bash

count=$(grep "Aggregate counter so far is" $1 | wc -l)
echo "The result count $2 is $count"

if [[ $count -eq 0 ]]; then
    exit 1
fi

## This checks if the Python interpreter finalized correctly.
## We don't have a proper testing framework, so this is a workaround.
## https://github.com/microsoft/jrt-controller/issues/56
# count=$(grep "Python interpreter finalized." $1 | wc -l)
# if [[ $count -eq 0 ]]; then
#     echo "Python interpreter did not finalize."
#     exit 1
# fi

### should not see: (core dumped) or Aborted
count=$(grep -c "Aborted" $1)
if [[ $count -ne 0 ]]; then
    echo "Found 'Aborted' in the log file"
    exit 1
fi
count=$(grep -c "core dumped" $1)
if [[ $count -ne 0 ]]; then
    echo "Found 'core dumped' in the log file"
    exit 1
fi

exit 0