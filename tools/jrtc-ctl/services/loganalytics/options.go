package loganalytics

import (
	"fmt"

	"github.com/spf13/pflag"
)

const (
	defaultLogType = "jrtc"
	optionsPrefix  = "loganalytics"
)

type options struct {
	logType            string
	timeGeneratedField string
}

func addOptionsToFlags(flags *pflag.FlagSet, opts *options) {
	if opts == nil {
		return
	}

	flags.StringVar(&opts.logType, optionsPrefix+"-log-type", defaultLogType, "log analytics log type")
	flags.StringVar(&opts.timeGeneratedField, optionsPrefix+"-time-generated-field", "", "the name of a field in the data that contains the timestamp of the data item. If you specify a field, its contents are used for TimeGenerated. If you don't specify this field, the default for TimeGenerated is the time that the message is ingested. ")
}

func (o *options) Parse() error {
	if o.logType == "" {
		return fmt.Errorf("%s must be set", optionsPrefix+"-log-type")
	}
	return nil
}
