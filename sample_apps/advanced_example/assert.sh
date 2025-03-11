#!/bin/bash

count=$(grep "Received aggregate counter" $1 | wc -l)
echo "The 'Received aggregate counter' count $2 is $count"
if [[ $count -eq 0 ]]; then
    exit 1
fi

exit 0
