// Copyright (c) Microsoft Corporation. All rights reserved.

package clicommon

import (
	"errors"
	"fmt"
	"os"

	"gopkg.in/yaml.v3"
)

var (
	// defaultForwardingDestination. Should align with jrtcbindings.DefaultDestination.String()
	defaultForwardingDestination = "DestinationNone"
)

// LinkedMapConfig is a linked map configuration
type LinkedMapConfig struct {
	LinkedCodeletName string `json:"linked_codelet_name" yaml:"linked_codelet_name" jsonschema:"required"`
	LinkedMapName     string `json:"linked_map_name" yaml:"linked_map_name" jsonschema:"required"`
	MapName           string `json:"map_name" yaml:"map_name" jsonschema:"required"`
}

// SerdeProtobufConfig is a serialization/deserialization protobuf configuration
type SerdeProtobufConfig struct {
	MessageName string `json:"msg_name" yaml:"msg_name" jsonschema:"required"`
	PackagePath string `json:"package_path" yaml:"package_path" jsonschema:"required"`
}

// SerdeConfig is a serialization/deserialization configuration
type SerdeConfig struct {
	FilePath string               `json:"file_path" yaml:"file_path" jsonschema:"required"`
	Protobuf *SerdeProtobufConfig `json:"protobuf" yaml:"protobuf"`
}

// IOChannelConfig is an input or output map configuration
type IOChannelConfig struct {
	ForwardDestination string       `json:"forward_destination,omitempty" yaml:"forward_destination" jsonschema:"default=DestinationNone,enum=DestinationAny,enum=DestinationNone,enum=DestinationUDP"`
	Name               string       `json:"name" yaml:"name" jsonschema:"required"`
	Serde              *SerdeConfig `json:"serde,omitempty" yaml:"serde"`
}

// JBPFCLICodeletConfig is a codelet configuration
type JBPFCLICodeletConfig struct {
	HookName         string             `json:"hook_name" yaml:"hook_name" jsonschema:"required"`
	InIOChannel      []*IOChannelConfig `json:"in_io_channel,omitempty" yaml:"in_io_channel"`
	LinkedMaps       []*LinkedMapConfig `json:"linked_maps,omitempty" yaml:"linked_maps"`
	Name             string             `json:"codelet_name" yaml:"codelet_name" jsonschema:"required"`
	CodeletPath      string             `json:"codelet_path" yaml:"codelet_path" jsonschema:"required"`
	OutIOChannel     []*IOChannelConfig `json:"out_io_channel,omitempty" yaml:"out_io_channel"`
	Priority         *uint32            `json:"priority,omitempty" yaml:"priority"`
	RuntimeThreshold *uint32            `json:"runtime_threshold,omitempty" yaml:"runtime_threshold"`
}

// JBPFCLIConfig is a codelet set configuration
type JBPFCLIConfig struct {
	Codelets []*JBPFCLICodeletConfig `json:"codelet_descriptor" yaml:"codelet_descriptor" jsonschema:"required"`
	ID       string                  `json:"codeletset_id" yaml:"codeletset_id" jsonschema:"required"`
}

// JBPFConfigFromYaml reads a CLIConfig from a YAML file
func JBPFConfigFromYaml(file string) (*JBPFCLIConfig, error) {
	fi, err := os.Stat(file)
	if err != nil {
		return nil, err
	} else if fi.IsDir() {
		return nil, fmt.Errorf(`expected "%s" to be a file, got directory`, file)
	}
	data, err := os.ReadFile(file)
	if err != nil {
		return nil, err
	}

	var cfg JBPFCLIConfig
	err = yaml.Unmarshal(data, &cfg)
	if err != nil {
		return nil, err
	}

	if err := cfg.expandEnvVars(); err != nil {
		return nil, err
	}

	if err := cfg.setDefaults(); err != nil {
		return nil, err
	}

	// Make sure that codelets with destinations other than none have a Serde configuration
	for _, codelet := range cfg.Codelets {
		for _, m := range codelet.InIOChannel {
			if m.ForwardDestination != defaultForwardingDestination && m.Serde == nil {
				return nil, fmt.Errorf("codelet %s has a non-none destination but no serde configuration", codelet.Name)
			}
		}

		for _, m := range codelet.InIOChannel {
			if m.ForwardDestination != defaultForwardingDestination && m.Serde == nil {
				return nil, fmt.Errorf("codelet %s has a non-none destination but no serde configuration", codelet.Name)
			}
		}
	}

	// Make sure that linked maps are not circular and that all linked maps can be loaded
	isQueued := make(map[string]struct{})
	for _, codelet := range cfg.Codelets {
		for _, l := range codelet.LinkedMaps {
			if l.LinkedCodeletName == codelet.Name {
				return nil, fmt.Errorf("codelet %s has a linked map to itself", codelet.Name)
			}
			if _, ok := isQueued[l.LinkedCodeletName]; !ok {
				return nil, fmt.Errorf("linked map %s for codelet %s referenced before its loaded", l.LinkedCodeletName, codelet.Name)
			}
		}
		isQueued[codelet.Name] = struct{}{}
	}

	return &cfg, nil
}

func (c *JBPFCLIConfig) expandEnvVars() error {
	/**
	* Don't expand CodeletPath and Serde.FilePath because the reverse proxy may have different
	* environment variables than the ctl tool
	 */

	for _, c := range c.Codelets {
		// c.CodeletPath = os.ExpandEnv(c.CodeletPath)
		if err := fileExists(os.ExpandEnv(c.CodeletPath)); err != nil {
			return err
		}

		for _, m := range c.InIOChannel {
			if m.Serde != nil {
				// m.Serde.FilePath = os.ExpandEnv(m.Serde.FilePath)
				if err := fileExists(os.ExpandEnv(m.Serde.FilePath)); err != nil {
					return err
				}

				if m.Serde.Protobuf != nil {
					m.Serde.Protobuf.PackagePath = os.ExpandEnv(m.Serde.Protobuf.PackagePath)

					if err := fileExists(m.Serde.Protobuf.PackagePath); err != nil {
						return err
					}
				}
			}

		}

		for _, m := range c.OutIOChannel {
			if m.Serde != nil {
				// m.Serde.FilePath = os.ExpandEnv(m.Serde.FilePath)
				if err := fileExists(os.ExpandEnv(m.Serde.FilePath)); err != nil {
					return err
				}

				if m.Serde.Protobuf != nil {
					m.Serde.Protobuf.PackagePath = os.ExpandEnv(m.Serde.Protobuf.PackagePath)

					if err := fileExists(m.Serde.Protobuf.PackagePath); err != nil {
						return err
					}
				}
			}
		}
	}
	return nil
}

func (c *JBPFCLIConfig) setDefaults() error {
	for _, codelet := range c.Codelets {
		for _, m := range codelet.InIOChannel {
			if len(m.ForwardDestination) == 0 {
				m.ForwardDestination = defaultForwardingDestination
			}
		}

		for _, m := range codelet.OutIOChannel {
			if len(m.ForwardDestination) == 0 {
				m.ForwardDestination = defaultForwardingDestination
			}
		}
	}

	return nil
}

func fileExists(file string) error {
	fi, err := os.Stat(file)
	if err != nil {
		return errors.Join(fmt.Errorf(`error stating file "%s"`, file), err)
	}
	if fi.IsDir() {
		return fmt.Errorf(`expected "%s" to be a file, got directory`, file)
	}
	return nil
}
