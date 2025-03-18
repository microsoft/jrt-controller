#!/bin/bash

count=$(grep "Aggregate counter so far is" $1 | wc -l)
echo "The 'Aggregate counter so far' count $2 is $count"
if [[ $count -eq 0 ]]; then
    exit 1
fi

exit 0