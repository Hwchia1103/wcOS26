# Assignment workflow

This workflow follows the [Assignment I HackMD specification](https://hackmd.io/@fLANt9b6TbWx5I3lYKkBow/S17gcgmKxl). Run the commands from this directory. Set your student ID on targets that need it; the kernel release must end in `-os-A151064`.

## 1. Start and check the environment

```sh
make up STUDENT_ID=A151064
make check-tools
make clone
```

The container mounts this project at `/workspace` (read-only) and keeps the kernel source and generated build files in `docker/home/ubuntu`.

## 2. Configure and build Linux v6.1

```sh
make configure STUDENT_ID=A151064
make kernel JOBS=4
```

`configure` starts with RISC-V `defconfig` when no config exists, sets `CONFIG_LOCALVERSION=-os-A151064`, disables automatic local-version suffixes, prepares the config, and prints `make kernelrelease`. The two syscall implementations and RISC-V syscall registrations are in the Linux source tree. Review your changes before committing and creating email-format patches with `git format-patch`.

## 3. Build and boot the supplied syscall tests

After implementing the syscalls and rebuilding the kernel:

```sh
make test-bins
make initramfs
make run
```

At the initramfs shell, run:

```sh
./test_revstr
dmesg | tail -n 20
./test_tempbuf
dmesg | tail -n 30
./verify_syscalls
```

`verify_syscalls` also checks error returns, duplicate removal, truncation, and empty output. Capture the four required program and `dmesg` screenshots for the report. Exit QEMU with `Ctrl+A`, then `X`.

The original `initramfs.cpio.gz` is preserved. The workflow extracts it under `/home/ubuntu/initramfs`, copies in the two static RISC-V test binaries, and writes a separate `/home/ubuntu/initramfs.cpio.gz` for QEMU.

## 4. Prepare and package submission files

Write your report in your own words and export it as `submission/A151064/A151064_report.pdf`. To create the patch yourself after reviewing the implementation, run these commands inside the container:

```sh
cd /home/ubuntu/linux
git switch -c hw1-a151064
git add arch/riscv/include/uapi/asm/unistd.h include/linux/syscalls.h kernel/Makefile kernel/hw1_syscalls.c
git commit
git format-patch -1
```

Then collect the config, package list, and patch file:

```sh
make collect-config STUDENT_ID=A151064
make collect-patches STUDENT_ID=A151064
make package-list STUDENT_ID=A151064
make package STUDENT_ID=A151064
```

The package target checks for the PDF, `.config`, package list, and at least one `.patch` before creating `HW1_A151064.zip`. The archive contains only `submission/A151064/`, matching the required submission layout. Review the contents before uploading to E3.

## Useful commands

```sh
make shell
make down
```
