ML-Accelerator SystemC Peripherals
=============================================

This repository contains three types of SystemC peripherals: systolic arrays, piecewise linear approximations, and systolic arrays that calculate piecewise linear approximations in each DPU. They can be configured using multiple variables in `CMakeLists.txt`. 
The `run_...` scripts start multiple KLEE executions at once. The parameters set in `CMakeLists.txt` are referenced in the first few lines to identify the testbenches to run.

# How to run Docker image

```bash
make docker-build
make docker
```

The Docker image is based on the klee image. It is possible to use different versions of KLEE (e.g. local custom versions) by modifying the first line of the Dockerfile. 

# How to use Docker container

```bash
./make.sh
./source/run_approx.sh
```
Similar for the other scripts.

# Bugfixes

## ASan Shadow Memory Range error
If the `Shadow memory range interleaves with an existing memory mapping. ASan cannot proceed correctly. ABORTING` error occurs,
it might be worth checking with `sudo cat /proc/sys/vm/mmap_rnd_bits` if the system works with entropy greater than 28 bits.
In that case, run `sudo sysctl vp.mmap_rnd_bits=28` to set to 28 bits entropy ([see here](https://github.com/google/sanitizers/issues/1614)).
Important: run on host system, not in the docker container.
This problem is fixed for LLVM 17.0.0, however the Docker file uses LLVM 11.
