# Building a Unikernel FaaS Template for Python applications

| Software | Version                                               |
|----------|-------------------------------------------------------|
| Unikraft | Atlas 0.13.1~85b83a74                                 |
| Kraftkit | [0.6.1-custom](https://github.com/alanpjohn/kraftkit) |
| Python   | 3.7.3                                                 |

As the python code is not compiled into the unikernel image but mounted into the unikernel during runtime, we can use this same image for all our python functions.

## Setting up the work directory

1. Untar `fs0.tar.gz` to get the filesystem containing python code to be mounted

```bash
tar -xvzf fs0.tar.gz
```

This will get you the filesystem with the python code to mounted

```bash
fs0
├── bin
├── dev
├── wrapper.py
├── include
├── lib
├── lib64
├── python-wheels
├── pyvenv.cfg
└── share
```

Insert your python code into `unikraft/fs0/lib/python3.7/app/`. There should be file named `function.py` at this location and this file must contain a function with the signature which will be called. The `wrapper.py` should be not altered..

```python
def handler(event)
```

2. Get unikraft core version `Atlas 0.13.1~85b83a74`

```bash
mkdir .unikraft
git clone https://github.com/unikraft/unikraft && cd unikraft
git reset --hard 85b83a74 
cd ../../
```

3. Get all unikraft lib dependencies 

```bash
sudo kraft pkg pull zlib lwip musl python3 --containerd-addr=/run/containerd/containerd.sock
```

You might not have to use `sudo` if you have permissions for the `containerd.sock`. The libs will be stored in `.unikraft/libs`

## Building and Packaging

1. Building the unikernel

```bash
sudo kraft prepare --containerd-addr=/run/containerd/containerd.sock
cp .config.default-x86_64 .config 
sudo kraft build --containerd-addr=/run/containerd/containerd.sock
```

2. Packaging the unikernel as an OCI image

```bash
sudo cp build/default_qemu-x86_64 build/default_kvm-x86_64
sudo KRAFTKIT_LOG_TYPE=basic kraft pkg --as=oci --oci-tag=app-python -v=fs0  --containerd-addr=/run/containerd/containerd.sock
```

We include the filesystem to be mounted as OCI layer using the `-v` option which is implemented in [this fork](https://github.com/alanpjohn/kraftkit) of `kraftkit`

You should be able to manage the created OCI image using `ctr` or `nerdctl`

```bash
ctr images ls
REF                                TYPE                                       DIGEST                                                                  SIZE     PLATFORMS  LABELS 
unikraft.org/app-python:latest    application/vnd.oci.image.manifest.v1+json sha256:b8cc493da67ccceceebc1abb99d3b5d29871285130a38c9569ffb42683482c3c 28.3 MiB kvm/x86_64 -      
```

You can push and pull this image to container registries that support the `application/vnd.oci.image.manifest`

## Running

### Running in workdir

```bash
sudo qemu-system-x86_64 \
             -fsdev local,id=myid,path=$(pwd)/fs0,security_model=none \
             -device virtio-9p-pci,fsdev=myid,mount_tag=fs0,disable-modern=on,disable-legacy=off \
             -kernel build/default_qemu-x86_64 \
             -enable-kvm \
             -m 1G \
             -nographic
```

### Running via OCI image

```bash
mkdir /tmp
sudo kraft pull unikraft.org/app-python:latest --containerd-addr=/run/containerd/containerd.sock
```

This will create the following files
```
└── unikraft
    ├── bin
    │   └── kernel
    └── fs0
        ├── bin
        ├── dev
        ├── function.py
        ├── include
        ├── lib
        ├── lib64
        ├── python-wheels
        ├── pyvenv.cfg
        └── share
```

- `unikraft/bin/kernel` : The unikernel build
- `unikraft/fs0` : Filesystem to be mounted

```bash
sudo qemu-system-x86_64 \
             -fsdev local,id=myid,path=unikraft/fs0,security_model=none \
             -device virtio-9p-pci,fsdev=myid,mount_tag=fs0,disable-modern=on,disable-legacy=off \
             -kernel unikraft/bin/kernel \
             -enable-kvm \
             -m 1G \
             -nographic
```

