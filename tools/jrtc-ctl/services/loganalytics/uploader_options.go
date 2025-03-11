package loganalytics

import (
	"fmt"
	"time"

	"github.com/spf13/pflag"
)

const (
	defaultFlushPeriod  = 5 * time.Second
	defaultMaxBatchSize = 30 << 20 // 30 MB
)

// UploaderOptions is the options for the log analytics uploader client
type UploaderOptions struct {
	client *options

	flushPeriod  time.Duration
	maxBatchSize uint64
	enabled      bool
}

// AddUploaderOptionsToFlags adds the uploader options to the flags
func AddUploaderOptionsToFlags(flags *pflag.FlagSet, opts *UploaderOptions) {
	if opts == nil {
		return
	}

	if opts.client == nil {
		opts.client = &options{}
	}

	addOptionsToFlags(flags, opts.client)

	flags.BoolVar(&opts.enabled, optionsPrefix+"-enabled", false, fmt.Sprintf("whether log analytics uploading is enabled, if set to true, %s and %s env vars are required", workspaceIDEnvVar, primaryKeyEnvVar))
	flags.DurationVar(&opts.flushPeriod, optionsPrefix+"-flush-period", defaultFlushPeriod, "log analytics maximum amount of time to buffer")
	flags.Uint64Var(&opts.maxBatchSize, optionsPrefix+"-max-batch-size", defaultMaxBatchSize, "log analytics maximum batch size in bytes")
}

// Parse parses the uploader options
func (o *UploaderOptions) Parse() error {
	return nil
}
