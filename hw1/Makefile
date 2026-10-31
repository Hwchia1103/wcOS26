SHELL := /bin/bash

STUDENT_ID ?= A151064
JOBS ?= 4
KERNEL_DIR ?= /home/ubuntu/linux
WORK_DIR ?= /home/ubuntu
INITRAMFS_DIR ?= $(WORK_DIR)/initramfs
INITRAMFS_IMAGE ?= $(WORK_DIR)/initramfs.cpio.gz
SUBMISSION_DIR ?= submission/$(STUDENT_ID)
ARCH := riscv
CROSS_COMPILE := riscv64-linux-gnu-

.PHONY: help up down shell check-tools clone configure kernel test-bins initramfs run \
	package-list collect-config collect-patches package clean-initramfs require-id

help:
	@printf '%s\n' \
	  'NYCU OS HW1 workflow (run from this directory):' \
	  '  make up STUDENT_ID=<ID>       Build and start the Docker container' \
	  '  make check-tools              Check RISC-V compiler and QEMU in container' \
	  '  make clone                   Clone Linux v6.1 into /home/ubuntu/linux' \
	  '  make configure STUDENT_ID=<ID> Configure RISC-V defconfig and local version' \
	  '  make kernel                  Build the RISC-V kernel Image' \
	  '  make test-bins               Cross-compile the supplied syscall tests' \
	  '  make initramfs               Add tests and repack a working initramfs' \
	  '  make run                     Boot QEMU (exit with Ctrl+A, then X)' \
	  '  make package-list STUDENT_ID=<ID> Save the container package list' \
	  '  make collect-config STUDENT_ID=<ID> Copy the kernel .config for submission' \
	  '  make collect-patches STUDENT_ID=<ID> Copy Linux format-patches for submission' \
	  '  make package STUDENT_ID=<ID> Validate required files and create HW1_<ID>.zip'

require-id:
	@test -n "$(STUDENT_ID)" || { echo 'Set STUDENT_ID, e.g. make configure STUDENT_ID=313551000' >&2; exit 2; }

up:
	docker compose up -d --build

down:
	docker compose down

shell:
	docker compose exec main bash

check-tools:
	docker compose exec main bash -lc 'riscv64-linux-gnu-gcc -v && qemu-system-riscv64 --version'

clone:
	docker compose exec main bash -lc 'if [ ! -d "$(KERNEL_DIR)/.git" ]; then git clone https://github.com/torvalds/linux --branch v6.1 --depth 1 "$(KERNEL_DIR)"; else echo "Kernel source already exists at $(KERNEL_DIR)"; fi'

configure: require-id
	docker compose exec main bash -lc 'cd "$(KERNEL_DIR)" && if [ ! -f .config ]; then make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) defconfig; fi && scripts/config --file .config --set-str CONFIG_LOCALVERSION "-os-$(STUDENT_ID)" --disable CONFIG_LOCALVERSION_AUTO && make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) olddefconfig && make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) prepare && make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) kernelrelease'

kernel:
	docker compose exec main bash -lc 'cd "$(KERNEL_DIR)" && make -j$(JOBS) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) Image'

test-bins:
	docker compose exec main bash -lc '$(CROSS_COMPILE)gcc -static -o "$(WORK_DIR)/test_revstr" /workspace/test_revstr.c && $(CROSS_COMPILE)gcc -static -o "$(WORK_DIR)/test_tempbuf" /workspace/test_tempbuf.c && $(CROSS_COMPILE)gcc -static -o "$(WORK_DIR)/verify_syscalls" /workspace/verify_syscalls.c'

clean-initramfs:
	docker compose exec main bash -lc 'rm -rf "$(INITRAMFS_DIR)" "$(INITRAMFS_IMAGE)"'

initramfs:
	docker compose exec main bash -lc 'mkdir -p "$(INITRAMFS_DIR)" && cd "$(INITRAMFS_DIR)" && gzip -dc /workspace/initramfs.cpio.gz | cpio -idmu && install -m 0755 "$(WORK_DIR)/test_revstr" . && install -m 0755 "$(WORK_DIR)/test_tempbuf" . && install -m 0755 "$(WORK_DIR)/verify_syscalls" . && find . -print0 | cpio --null -o -H newc | gzip -c > "$(INITRAMFS_IMAGE)"'

run:
	docker compose exec -it main bash -lc 'cd "$(KERNEL_DIR)" && qemu-system-riscv64 -nographic -machine virt -kernel arch/riscv/boot/Image -initrd "$(INITRAMFS_IMAGE)" -append "console=ttyS0 loglevel=3"'

package-list: require-id
	mkdir -p "$(SUBMISSION_DIR)"
	docker compose exec main dpkg --get-selections > "$(SUBMISSION_DIR)/packages_list_$(STUDENT_ID).txt"

collect-config: require-id
	mkdir -p "$(SUBMISSION_DIR)"
	docker compose exec main bash -lc 'test -f "$(KERNEL_DIR)/.config" && cp "$(KERNEL_DIR)/.config" "$(WORK_DIR)/hw1.config"'
	cp docker/home/ubuntu/hw1.config "$(SUBMISSION_DIR)/.config"

collect-patches: require-id
	@test -d docker/home/ubuntu/linux || { echo 'Kernel tree not found in the Docker workspace' >&2; exit 1; }
	@mkdir -p "$(SUBMISSION_DIR)"
	@find docker/home/ubuntu/linux -maxdepth 1 -type f -name '*.patch' -exec cp {} "$(SUBMISSION_DIR)/" \;
	@test -n "$$(find "$(SUBMISSION_DIR)" -maxdepth 1 -type f -name '*.patch' -print -quit)" || { echo 'No .patch files found; create them with git format-patch first.' >&2; exit 1; }

package: require-id
	@test -f "$(SUBMISSION_DIR)/$(STUDENT_ID)_report.pdf" || { echo 'Add the required PDF report first.' >&2; exit 1; }
	@test -f "$(SUBMISSION_DIR)/.config" || { echo 'Run make collect-config first.' >&2; exit 1; }
	@test -f "$(SUBMISSION_DIR)/packages_list_$(STUDENT_ID).txt" || { echo 'Run make package-list first.' >&2; exit 1; }
	@test -n "$$(find "$(SUBMISSION_DIR)" -maxdepth 1 -type f -name '*.patch' -print -quit)" || { echo 'Add git format-patch output first.' >&2; exit 1; }
	rm -f "HW1_$(STUDENT_ID).zip"
	cd submission && zip -r "../HW1_$(STUDENT_ID).zip" "$(STUDENT_ID)"
