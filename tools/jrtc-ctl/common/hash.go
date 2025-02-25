// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package common

import (
	"crypto/sha1"
	"fmt"
	"strconv"
)

// GenerateHashFromStrings generates a UUID from the given strings.
func GenerateHashFromStrings(parts ...string) ([]byte, error) {
	if len(parts) == 0 {
		return nil, fmt.Errorf("stream ID components cannot be nil")
	}

	s := ""
	for i, x := range parts {
		s += strconv.Itoa(i)
		s += x
	}
	h := sha1.New()
	h.Write([]byte(s))
	hash := h.Sum(nil)

	return hash[:16], nil
}
