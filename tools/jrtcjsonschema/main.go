// Copyright (c) Microsoft Corporation. All rights reserved.

package main

import (
	"jrtc-ctl/cmd/clicommon"
	"encoding/json"
	"log"
	"os"
	"path/filepath"

	"github.com/spf13/pflag"

	"github.com/invopop/jsonschema"
)

var (
	outputDir string
	r         = new(jsonschema.Reflector)
)

func init() {
	pflag.StringVarP(&outputDir, "output-dir", "o", "./", "output directory for generated schemas")
	pflag.Parse()

	if err := r.AddGoComments("deployment", "../jrtc-ctl/cmd/clicommon"); err != nil {
		log.Fatal(err)
	}
}

func main() {
	if err := generateJSONSchema(&clicommon.CLIConfig{}, "jrtc-ctl.schema.json"); err != nil {
		log.Fatal(err)
	}
	if err := generateJSONSchema(&clicommon.JBPFCLIConfig{}, "jbpf.schema.json"); err != nil {
		log.Fatal(err)
	}
}

func generateJSONSchema(v any, filename string) error {
	absOutputDir, err := filepath.Abs(outputDir)
	if err != nil {
		return err
	}
	s := r.Reflect(v)
	filePath := filepath.Join(absOutputDir, filename)
	data, err := json.MarshalIndent(s, "", "  ")
	if err != nil {
		return err
	}

	f, err := os.Create(filePath)
	if err != nil {
		return err
	}

	_, err = f.Write(data)
	return err
}
