# jrtc-ctl command line interface

CLI tool to load/unload jrt-controller components.

The cli tool can load an entire *jrt-controller* bundle or at each individual layer:
* Jbpf codeletsets
* jrt-controller applications
* eBPF codelets
And optionally send schemas to a local or remote decoder.
For more on its architecture and goals, see [here](../../docs/jrtctl.md).

## Usage

See `jrtc-ctl --help` for detailed usage.

## Build locally

Prerequisites:
* Go v1.21+
* Make
* Cmake

Setup:
1. Checkout jbpf and build from source
2. Build jrt-controller from source
3. From jbpf root `source setup_jbpf_env.sh`
3. From jrt-controller root `source setup_jrtc_env.sh`

```sh
# Download packages and generate assets
make mod generate

# Build
make

# (Optionally) test and lint the project
make test lint -j
```

