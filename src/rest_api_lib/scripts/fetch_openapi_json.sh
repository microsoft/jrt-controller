#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd $(dirname $CURRENT_DIR)
rm -f openapi.json
wget -q http://localhost:3000/api-docs/openapi.json
cp openapi.json openapi.json.tmp
jq . openapi.json.tmp >openapi.json
rm openapi.json.tmp
