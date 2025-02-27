# jrt-controller JSON Schema

*Note: this code is called from a `go:generate` script in jrtc-ctl [](../jrtc-ctl/cmd/clicommon/config.go)*

Utility tool to generate [jrtc-ctl.schema.json](../schemas/jrtc-ctl.schema.json) which is the [JSON Schema](https://json-schema.org/) for the jrt-controller api configuration files.

Prerequisites:
* Go v1.21+

## Usage

```sh
go mod tidy
go run ./... -o ../schemas
```
