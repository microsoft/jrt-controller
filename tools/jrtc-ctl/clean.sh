# Copyright (c) Microsoft Corporation. All rights reserved.
#!/bin/bash

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

rm -f $(dirname $CURRENT_DIR)/schemas/*.json \
	services/*/*.gen.go services/*/*.pb.go \
	services/*/*.pb.gw.go
