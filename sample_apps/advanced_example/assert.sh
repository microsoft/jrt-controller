#!/bin/bash

count=$(grep "Aggregate counter so far is" $1 | wc -l)
echo "The 'Aggregate counter so far' count $2 is $count"
if [[ $count -eq 0 ]]; then
    exit 1
fi

count=$(grep "Received aggregate counter" $1 | wc -l)
echo "The 'Received aggregate counter' count $2 is $count"
if [[ $count -eq 0 ]]; then
    exit 1
fi

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
