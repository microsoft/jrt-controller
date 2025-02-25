package common

import (
	"encoding/hex"
	"text/template"
)

// Funcs is the common template functions
var Funcs template.FuncMap

func init() {
	Funcs = template.FuncMap{}

	Funcs["bytesAsHex"] = func(src []byte) (out string) {
		return hex.EncodeToString(src)
	}
}
