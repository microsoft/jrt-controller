#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

rm -f $(dirname $CURRENT_DIR)/schemas/*.json \
	services/*/*.gen.go services/*/*.pb.go \
	services/*/*.pb.gw.go
