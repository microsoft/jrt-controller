#!/bin/bash

count=$(grep "FirstExample: Aggregate counter so far is" $1 | wc -l)
echo "The result count $2 is $count"

if [[ $count -eq 0 ]]; then
    exit 1
fi

count=$(grep "SecondExample: Aggregate counter so far is" $1 | wc -l)
echo "The result count $2 is $count"

if [[ $count -eq 0 ]]; then
    exit 1
fi

exit 0