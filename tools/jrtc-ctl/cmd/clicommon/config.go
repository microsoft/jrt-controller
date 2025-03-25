// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package clicommon

import (
	"encoding/json"
	"errors"
	"fmt"
	"net"
	"os"
	"strings"
	"time"

	"gopkg.in/yaml.v3"
)

const (
	defaultAppDeadline = 0 * time.Second
	defaultAppIOQSize  = int32(1000)
	defaultAppIP       = "127.0.0.1"
	defaultAppPeriod   = 0 * time.Second
	defaultAppRuntime  = 0 * time.Second
	defaultDecoderIP   = "127.0.0.1"
	defaultDeviceIP    = "127.0.0.1"
	defaultDecoderType = "decodergrpc"
)

//go:generate go run -C ../../../jrtcjsonschema ./... -o ../schemas

// Decoder receives output from jrt-controller and decodes it, the type of decoder can be set to: (1) decodergrpc for a local debugger using "jrtc-ctl decoder" command, or (2) decoderhttp for a debugger using "jrtc-ctl decoder" command with http gateway
type Decoder struct {
	HTTPRelativePath string `json:"http_relative_path,omitempty" jsonschema:"default=,omitempty"`
	IP               string `json:"-"`
	OptionalIP       net.IP `json:"ip,omitempty" jsonschema:"default=127.0.0.1"`
	Port             uint16 `json:"port" jsonschema:"required,minimum=0,maximum=65535"`
	Typ              string `json:"type" jsonschema:"default=decodergrpc,enum=decodergrpc,enum=decoderhttp"`
}

// JBPFDevice is an agent that can run JBPF Codelets
type JBPFDevice struct {
	ID         uint8  `json:"id" jsonschema:"required,minimum=0,maximum=127"`
	IP         string `json:"-"`
	OptionalIP net.IP `json:"ip,omitempty" jsonschema:"default=127.0.0.1"`
	Port       uint16 `json:"port" jsonschema:"required,minimum=0,maximum=65535"`
}

// App represents an application
type App struct {
	Deadline          time.Duration `json:"-"`
	IOQSize           int32         `json:"-"`
	IP                string        `json:"-"`
	Name              string        `json:"name" jsonschema:"required"`
	OptionalDeadline  *string       `json:"deadline,omitempty" jsonschema:"default=0s"`
	OptionalIOQSize   *int32        `json:"ioq_size,omitempty" jsonschema:"default=1000"`
	OptionalIP        net.IP        `json:"ip,omitempty" jsonschema:"default=127.0.0.1"`
	OptionalPeriod    *string       `json:"period,omitempty" jsonschema:"default=0s"`
	OptionalRuntime   *string       `json:"runtime,omitempty" jsonschema:"default=0s"`
	Period            time.Duration `json:"-"`
	Port              uint16        `json:"port" jsonschema:"required,minimum=0,maximum=65535"`
	Runtime           time.Duration `json:"-"`
	SharedLibraryCode []byte        `json:"-"`
	SharedLibraryPath string        `json:"path" jsonschema:"required"`
	AppType           string        `json:"type" jsonschema:"required"` // type is a string e.g. "c" or "python"
	// params is a dictionary e.g. key value pairs
	AppParams map[string]interface{} `json:"params,omitempty"`
	// modules is a list of strings e.g. ["module1", "module2"]
	AppModules []string `json:"modules,omitempty"`
}

// JBPFCodelet represents a JBPF codelet
type JBPFCodelet struct {
	ConfigFile string `json:"config" jsonschema:"required"`
	Device     uint8  `json:"device" jsonschema:"required"`
}

// JBPFConfig is the configuration for JBPF
type JBPFConfig struct {
	Devices      []*JBPFDevice  `json:"device,omitempty"`
	JBPFCodelets []*JBPFCodelet `json:"codelet_set,omitempty"`
}

// GetDeviceMap returns a map of device addr to device ID
func (c *JBPFConfig) GetDeviceMap() map[string]uint8 {
	out := make(map[string]uint8)
	for _, d := range c.Devices {
		out[fmt.Sprintf("%s:%d", d.IP, d.Port)] = d.ID
	}
	return out
}

// CLIConfig represents a configuration
type CLIConfig struct {
	Apps     []*App      `json:"app,omitempty"`
	Decoders []*Decoder  `json:"decoder,omitempty"`
	JBPF     *JBPFConfig `json:"jbpf,omitempty"`
	Name     string      `json:"name" jsonschema:"required"`
}

// FromYamls reads multiple YAML files and merges them into a single CLIConfig
func FromYamls(files ...string) (*CLIConfig, error) {
	if len(files) == 0 {
		return nil, errors.New("no files provided")
	}
	out := (*CLIConfig)(nil)

	for _, file := range files {
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

		var yamlConf interface{}
		if err := yaml.Unmarshal(data, &yamlConf); err != nil {
			return nil, errors.Join(err, fmt.Errorf("failed to unmarshal %s as %T", file, yamlConf))
		}

		jsonBuf, err := json.Marshal(yamlConf)
		if err != nil {
			return nil, err
		}

		var innerConf CLIConfig
		if err := json.Unmarshal(jsonBuf, &innerConf); err != nil {
			return nil, errors.Join(err, fmt.Errorf("failed to unmarshal %s as %T", file, innerConf))
		}

		combinedConf, err := mergeConfigs(out, &innerConf)
		if err != nil {
			return nil, err
		}
		out = combinedConf
	}

	out.expandEnvVars()

	if err := out.setDefaults(); err != nil {
		return nil, errors.Join(err, fmt.Errorf("failed to set defaults for %s", strings.Join(files, ", ")))
	}

	if err := out.validate(); err != nil {
		return nil, errors.Join(err, fmt.Errorf("failed to validate %s", strings.Join(files, ", ")))
	}

	return out, nil
}

func mergeConfigs(c1 *CLIConfig, c2 *CLIConfig) (*CLIConfig, error) {
	if c1 == nil {
		return c2, nil
	} else if c2 == nil {
		return c1, nil
	}

	if c1.Name != c2.Name {
		return nil, fmt.Errorf("expected name %q, got %q", c1.Name, c2.Name)
	}

	apps := append(c1.Apps, c2.Apps...)
	decoders := append(c1.Decoders, c2.Decoders...)

	devices := make([]*JBPFDevice, 0)
	codeletsets := make([]*JBPFCodelet, 0)
	if c1.JBPF != nil && c2.JBPF != nil {
		devices = append(c1.JBPF.Devices, c2.JBPF.Devices...)
		codeletsets = append(c1.JBPF.JBPFCodelets, c2.JBPF.JBPFCodelets...)
	} else if c1.JBPF != nil {
		devices = c1.JBPF.Devices
		codeletsets = c1.JBPF.JBPFCodelets
	} else if c2.JBPF != nil {
		devices = c2.JBPF.Devices
		codeletsets = c2.JBPF.JBPFCodelets
	}
	return &CLIConfig{
		Apps:     apps,
		Decoders: decoders,
		JBPF: &JBPFConfig{
			Devices:      devices,
			JBPFCodelets: codeletsets,
		},
		Name: c1.Name,
	}, nil
}

func (c *CLIConfig) setDefaults() (err error) {
	for _, app := range c.Apps {
		if app.OptionalDeadline != nil {
			app.Deadline, err = time.ParseDuration(*app.OptionalDeadline)
			if err != nil {
				return
			}
		} else {
			app.Deadline = defaultAppDeadline
		}
		if app.OptionalPeriod != nil {
			app.Period, err = time.ParseDuration(*app.OptionalPeriod)
			if err != nil {
				return
			}
		} else {
			app.Period = defaultAppPeriod
		}
		if app.OptionalRuntime != nil {
			app.Runtime, err = time.ParseDuration(*app.OptionalRuntime)
			if err != nil {
				return
			}
		} else {
			app.Runtime = defaultAppRuntime
		}

		app.IP = defaultAppIP
		if app.OptionalIP != nil && app.OptionalIP.String() != "" {
			app.IP = app.OptionalIP.String()
		}
		if app.OptionalIOQSize == nil {
			app.IOQSize = defaultAppIOQSize
		} else {
			app.IOQSize = *app.OptionalIOQSize
		}
	}

	for _, dec := range c.Decoders {
		dec.IP = defaultDecoderIP
		if dec.OptionalIP != nil && dec.OptionalIP.String() != "" {
			dec.IP = dec.OptionalIP.String()
		}
	}

	for _, dev := range c.JBPF.Devices {
		dev.IP = defaultDeviceIP
		if dev.OptionalIP != nil && dev.OptionalIP.String() != "" {
			dev.IP = dev.OptionalIP.String()
		}
	}

	return nil
}

func (c *CLIConfig) expandEnvVars() {
	for _, app := range c.Apps {
		app.SharedLibraryPath = os.ExpandEnv(app.SharedLibraryPath)
		for i := range app.AppModules {
			app.AppModules[i] = os.ExpandEnv(app.AppModules[i])
		}
		for k, v := range app.AppParams {
			switch v := v.(type) {
			case string:
				app.AppParams[k] = os.ExpandEnv(v)
			case []string:
				for i := range v {
					v[i] = os.ExpandEnv(v[i])
				}
				app.AppParams[k] = v
			}
		}
		for i := range app.AppModules {
			app.AppModules[i] = os.ExpandEnv(app.AppModules[i])
		}
	}
	for _, c := range c.JBPF.JBPFCodelets {
		c.ConfigFile = os.ExpandEnv(c.ConfigFile)
	}
}

func (c *CLIConfig) validate() error {
	if c.JBPF != nil {
		seenDevices := make(map[uint8]struct{})
		usedDevices := make(map[uint8]struct{})
		for _, device := range c.JBPF.Devices {
			if _, ok := seenDevices[device.ID]; ok {
				return fmt.Errorf("device %d already defined", device.ID)
			}
			seenDevices[device.ID] = struct{}{}
		}
		for _, jc := range c.JBPF.JBPFCodelets {
			usedDevices[jc.Device] = struct{}{}
			if _, ok := seenDevices[jc.Device]; !ok {
				return fmt.Errorf("device %d not found", jc.Device)
			}
		}
		for d := range usedDevices {
			delete(seenDevices, d)
		}
		if len(seenDevices) > 0 {
			deviceIDs := make([]string, 0, len(seenDevices))
			for id := range seenDevices {
				deviceIDs = append(deviceIDs, fmt.Sprint(id))
			}
			return fmt.Errorf("unused devices: %v", strings.Join(deviceIDs, ", "))
		}
	}

	return nil
}
