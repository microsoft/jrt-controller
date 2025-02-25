// Copyright (c) Microsoft Corporation. All rights reserved.

package common

import (
	"errors"

	"github.com/sirupsen/logrus"
	"github.com/spf13/pflag"
)

// NewGeneralOptions creates a new GeneralOptions with default values
func NewGeneralOptions(flags *pflag.FlagSet) *GeneralOptions {
	opts := &GeneralOptions{}
	opts.addToFlags(flags)
	return opts
}

// GeneralOptions contains the general options for the jrtc-cli
type GeneralOptions struct {
	logLevel     string
	reportCaller bool

	Logger *logrus.Logger
}

func (opts *GeneralOptions) addToFlags(flags *pflag.FlagSet) {
	flags.BoolVar(&opts.reportCaller, "log-report-caller", false, "show report caller in logs")
	flags.StringVar(&opts.logLevel, "log-level", "info", "log level, set to: panic, fatal, error, warn, info, debug or trace")
}

// Parse will process and validate args
func (opts *GeneralOptions) Parse() error {
	var err1, err2 error
	opts.Logger, err1 = opts.getLogger()
	return errors.Join(err1, err2)
}

// GetLogger returns a logger based on the options
func (opts *GeneralOptions) getLogger() (*logrus.Logger, error) {
	logger := logrus.New()
	logLev, err := logrus.ParseLevel(opts.logLevel)
	if err != nil {
		return logger, err
	}

	logger.SetReportCaller(opts.reportCaller)
	logger.SetLevel(logLev)
	return logger, nil
}
