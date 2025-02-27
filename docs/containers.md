# Building with containers

We provide standard Dockerfiles to build *jrt-controller* images for standard operating systems. 

## Images

The Dockerfiles in this directory can be split into two categories:
1. builder images for different operating systems:
  * [azurelinux](../deploy/azurelinux.Dockerfile)
  * [Ubuntu22.04](../deploy/ubuntu22_04.Dockerfile)
  * [Ubuntu24.04](../deploy/ubuntu24_04.Dockerfile)
2. Containerized application
  * [jrtc-ctl](../deploy/jrtc-ctl.Dockerfile) is built from one of the OS images in step 1.

## Build

```sh
# To build for a particular OS, run:
OS=azurelinux  # possible values: [ubuntu22_04, azurelinux, ubuntu24_04]

# Create builder image with all dependencies loaded
sudo -E docker build -t jrtc-$OS:latest -f deploy/$OS.Dockerfile .

# Build the jrt-controller
sudo -E docker run -v `pwd`:/jrtc_out_lib --rm jrtc-$OS:latest
```

With the commands above, you should find the compiled binaries at `$(PWD)/out` directory.


### Build jrtc-ctl
And to create a jrtc-ctl image from that container, run:

```sh
sudo -E docker build --build-arg builder_image=jrtc-$OS --build-arg builder_image_tag=latest -t jrtcctl:latest - < deploy/jrtc-ctl.Dockerfile
```