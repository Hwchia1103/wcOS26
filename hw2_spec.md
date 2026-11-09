# Assignment II Specification: System Information Fetching Kernel Module (kfetch)
**[NYCU CSIC30015 Operating Systems] by Prof. Chun-Feng Wu**
**Curated & Supplemented by Senior Course Teaching Assistant**

- **Target Directory**: `HW3-student`
- **Reference Spec**: [HackMD: System Information Fetching Kernel Module](https://hackmd.io/@seco1024/BJYIDp7lZx)
- **Student ID**: `A151064`
- **Output Artifacts**: `hw2_spec.md` (Specification & Checklist), `hw2_report.md` (Final Project Report)

---

## 1. Overview & Learning Objectives

The primary goal of this assignment is to develop an in-depth understanding of the **Linux Kernel Architecture**, specifically regarding **Loadable Kernel Modules (LKMs)**, the **Character Device Subsystem**, **In-Kernel Telemetry Extraction**, and **User-Kernel Space Boundary Crossing**.

Students are required to implement a Linux character device driver named `kfetch_mod` (creating `/dev/kfetch`) and pair it with a user-space utility `kfetch` (similar to `neofetch` / `fastfetch`). Unlike user-space tools that parse virtual files in `/proc` or `/sys`, `kfetch_mod` fetches live hardware and execution statistics directly from in-memory kernel structures and formats the results alongside an ASCII art mascot.

### Key Competencies Developed:
1. **Kernel Compilation & Environment Control**: Building, configuring, and verifying a custom Linux v6.1 LTS kernel from source.
2. **Character Device Driver Architecture**: Dynamic allocation of device numbers (`alloc_chrdev_region`), `cdev` management, and sysfs auto-population (`class_create`, `device_create`).
3. **Internal Kernel API Utilization**: Direct querying of kernel subsystems for OS version (`utsname`), processor telemetry (`boot_cpu_data`), memory state (`si_meminfo`), process tables (`for_each_process`), and monotonic clocks (`ktime_get_boottime_ts64`).
4. **Data Isolation & Concurrency Control**: Safe memory transfers (`copy_from_user`, `copy_to_user`), offset/EOF handling in `read()`, and race-condition prevention using kernel mutexes (`struct mutex`).

---

## 2. Environment Setup & Kernel Compilation (Do It Yourself!)

> [!NOTE]
> The original HackMD specification marks the kernel compilation step as *"(Do It Yourself!)"*. This section provides the exhaustive, step-by-step engineering instructions required to build and verify the Linux v6.1 LTS kernel baseline.

### 2.1 Essential Build Toolchain Installation
To compile a modern Linux kernel on Debian/Ubuntu-based distributions, install the following build dependencies:

```bash
sudo apt update && sudo apt install -y \
    build-essential \
    libncurses-dev \
    bison \
    flex \
    libssl-dev \
    libelf-dev \
    dwarves \
    bc \
    cpio \
    rsync \
    git \
    kmod
```

### 2.2 Source Tree Acquisition (Linux v6.1 LTS)
Clone the official Linux stable tree focusing on the Long Term Support (LTS) 6.1 branch:

```bash
# Clone with depth 1 to save disk space and bandwidth
git clone https://github.com/torvalds/linux --branch v6.1 --depth 1 ~/linux-6.1
cd ~/linux-6.1
```

### 2.3 Kernel Configuration & Suffix Customization
1. **Generate Baseline Configuration**:
   ```bash
   # Copy existing host config if available, or generate default config
   if [ -f /boot/config-$(uname -r) ]; then
       cp /boot/config-$(uname -r) .config
       make olddefconfig
   else
       make defconfig
   fi
   ```

2. **Crucial Fix for Distribution Key Errors**:
   Standard Ubuntu/Debian configurations enable trusted module signing keys that do not exist locally, causing compilation failures. Disable them explicitly:
   ```bash
   scripts/config --file .config --set-str CONFIG_SYSTEM_TRUSTED_KEYS ""
   scripts/config --file .config --set-str CONFIG_SYSTEM_REVOCATION_KEYS ""
   ```

3. **Set Student Local Version**:
   ```bash
   scripts/config --file .config --set-str CONFIG_LOCALVERSION "-os-A151064"
   scripts/config --file .config --disable CONFIG_LOCALVERSION_AUTO
   make olddefconfig
   ```

4. **Verify Target Release String**:
   ```bash
   make kernelrelease
   # Expected Output: 6.1.0-os-A151064 (or 6.1.x-os-A151064)
   ```

### 2.4 Compilation & Installation
```bash
# Compile kernel binary and modules leveraging all CPU cores
make -j$(nproc)

# Install modules to /lib/modules/<kernelrelease>
sudo make modules_install

# Install kernel image, System.map, and update GRUB bootloader
sudo make install
sudo update-grub
```

### 2.5 Reboot & Validation
Reboot the machine (or boot QEMU if testing in an emulator):
```bash
sudo reboot
```
Verify the active kernel version:
```bash
uname -r
# Output MUST contain '6.1' and '-os-A151064'
```

---

## 3. Character Device Architecture Specification

The kernel module must expose an interface to user-space applications through a standard character device located at `/dev/kfetch`.

```
+-------------------------------------------------------------+
|                      User Space                             |
|  ./kfetch [options]                                         |
|    |                                                        |
|    | open("/dev/kfetch", O_RDWR)                            |
|    | write(fd, &mask, sizeof(mask))                         |
|    | read(fd, buf, size)                                    |
|    v                                                        |
+-------------------------------------------------------------+
|                      Kernel Space                           |
|  VFS (Virtual File System)                                  |
|    |                                                        |
|    v                                                        |
|  kfetch_fops:                                               |
|    .open    = kfetch_open                                   |
|    .release = kfetch_release                                |
|    .write   = kfetch_write  ---> copy_from_user(&mask)      |
|    .read    = kfetch_read   ---> Gather Stats & Tux ASCII   |
|                                  copy_to_user(buf)          |
|  Concurrency Protection: DEFINE_MUTEX(kfetch_mutex)         |
+-------------------------------------------------------------+
```

### 3.1 Device Number Allocation & `cdev` Registration
- **Device Name**: `kfetch` (`#define KFETCH_DEV_NAME "kfetch"`).
- **Dynamic Allocation**: Allocate dynamic major/minor numbers using `alloc_chrdev_region(&dev_num, 0, 1, KFETCH_DEV_NAME)`. Static major numbers are strictly prohibited to prevent conflicts.
- **Cdev Binding**: Initialize `struct cdev` using `cdev_init(&kfetch_cdev, &kfetch_fops)` and register it via `cdev_add(&kfetch_cdev, dev_num, 1)`.

### 3.2 Automated Device Node Creation via Sysfs
Manual creation via `mknod` is deprecated. The driver must register with `sysfs` and `udev` to automatically instantiate `/dev/kfetch`:
- **Class Creation**:
  ```c
  #if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
      kfetch_class = class_create(KFETCH_DEV_NAME);
  #else
      kfetch_class = class_create(THIS_MODULE, KFETCH_DEV_NAME);
  #endif
  ```
- **Device Node**:
  ```c
  kfetch_device = device_create(kfetch_class, NULL, dev_num, NULL, KFETCH_DEV_NAME);
  ```

### 3.3 File Operations Handlers
The driver implements `struct file_operations`:
1. **`kfetch_open(struct inode *inode, struct file *file)`**:
   - Invoked when user space calls `open("/dev/kfetch", ...)`.
   - Logs device access to the kernel ring buffer (`pr_info`).
   - Returns `0` on success.

2. **`kfetch_release(struct inode *inode, struct file *file)`**:
   - Invoked upon `close(fd)`.
   - Cleans up any per-file state and logs the closure (`pr_info`).
   - Returns `0` on success.

3. **`kfetch_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos)`**:
   - Receives an integer bitmask (`int mask`) configuring which telemetry fields should be displayed.
   - Verifies input size (`count >= sizeof(int)`).
   - Transfers data securely from user space using `copy_from_user()`. Returns `-EFAULT` on copy failure.
   - Updates module-wide mask state protected by `mutex_lock(&kfetch_mutex)`.
   - Returns the number of bytes consumed (`sizeof(int)`).

4. **`kfetch_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)`**:
   - Gathers live system metrics according to the currently active mask.
   - Formats the Tux ASCII logo and telemetry into a contiguous text buffer.
   - **Crucial EOF Handling**: Inspects `*f_pos`. If `*f_pos > 0`, immediately returns `0` (indicating End-Of-File). Neglecting this check causes `read()` loops in user space to block or print repeatedly.
   - Transfers data safely to user space using `copy_to_user()`.
   - Advances `*f_pos` by the number of bytes transferred and returns the byte count.

### 3.4 Concurrency Control & Synchronization
- Because multiple processes may open, configure, and read `/dev/kfetch` concurrently, shared global states (such as the active information mask and the output scratch buffer) must be protected using a kernel mutex:
  ```c
  static DEFINE_MUTEX(kfetch_mutex);
  ```
- All access to `mask_info` and buffer generation logic must be enclosed within `mutex_lock(&kfetch_mutex)` and `mutex_unlock(&kfetch_mutex)`.

---

## 4. In-Kernel System Telemetry Extraction Specifications

The module must query authoritative kernel data structures directly rather than emulating user-space file parsers:

| Metric | Kernel API / Data Structure | Required Header | Description & Calculation |
| :--- | :--- | :--- | :--- |
| **Kernel Release** | `utsname()->release` | `<linux/utsname.h>` | Returns kernel release string (e.g. `6.1.0-os-A151064`). |
| **Hostname** | `utsname()->nodename` | `<linux/utsname.h>` | System hostname displayed as header banner. |
| **CPU Model** | `boot_cpu_data.x86_model_id` | `<asm/processor.h>` | CPU brand and model string (portable fallback for RISC-V). |
| **CPU Cores** | `num_online_cpus()`, `num_possible_cpus()` | `<linux/cpumask.h>` | Number of active online CPU cores vs total cores. |
| **Memory Usage** | `si_meminfo(struct sysinfo *si)` | `<linux/mm.h>` | `totalram` and `freeram` converted to MB: `(si.ram * si.mem_unit) / (1024 * 1024)`. |
| **System Uptime** | `ktime_get_boottime_ts64(&ts)` | `<linux/timekeeping.h>` | Monotonic boot time converted to integer minutes: `ts.tv_sec / 60`. |
| **Process Count** | `for_each_process(struct task_struct *p)` | `<linux/sched/signal.h>` | Protected under `rcu_read_lock()` / `rcu_read_unlock()` to count running tasks. |

---

## 5. Information Masking & User-Space Tooling

### 5.1 Bitwise Flag Definitions (`kfetch.h`)
The information mask is represented as a 32-bit integer with discrete bit positions:

```c
#define KFETCH_DEV_NAME "kfetch"
#define KFETCH_DEV_PATH "/dev/kfetch"
#define KFETCH_BUF_SIZE 1024

#define KFETCH_NUM_INFO 6

#define KFETCH_RELEASE   (1 << 0) /* 0x01: Kernel release */
#define KFETCH_NUM_CPUS  (1 << 1) /* 0x02: Number of CPU cores */
#define KFETCH_CPU_MODEL (1 << 2) /* 0x04: CPU model name */
#define KFETCH_MEM       (1 << 3) /* 0x08: Free / Total memory */
#define KFETCH_UPTIME    (1 << 4) /* 0x10: System uptime in minutes */
#define KFETCH_NUM_PROCS (1 << 5) /* 0x20: Active process count */

#define KFETCH_FULL_INFO ((1 << KFETCH_NUM_INFO) - 1) /* 0x3F: All fields */
```

### 5.2 Command-Line Interface (`kfetch`) Options
The user-space utility parses standard POSIX options via `getopt`:
- `-a`: Show all information (`mask = KFETCH_FULL_INFO`).
- `-c`: Show CPU model name (`mask |= KFETCH_CPU_MODEL`).
- `-m`: Show memory statistics (`mask |= KFETCH_MEM`).
- `-n`: Show the count of CPU cores (`mask |= KFETCH_NUM_CPUS`).
- `-p`: Show the count of active processes (`mask |= KFETCH_NUM_PROCS`).
- `-r`: Show the kernel release string (`mask |= KFETCH_RELEASE`).
- `-u`: Show how long the system has been running (`mask |= KFETCH_UPTIME`).
- If no options are specified, `kfetch` does not execute `write()`, relying instead on the module's initialized or retained mask.

### 5.3 Output Layout & ASCII Tux Logo
The character device outputs a combined text stream formatted in two adjacent columns:
1. **Left Column (20 characters wide)**: ASCII Tux penguin artwork.
2. **Right Column**: Host banner, divider line, and filtered system metrics.

```text
                    chia@sandbox
        .-.         ------------
       (.. |        Kernel:   6.1.0-os-A151064
       <>_\|        CPU:      AMD EPYC 7763 64-Core Processor
      / --- \       CPUs:     4 / 4
     ( |   | )      Mem:      6120 MB / 7924 MB
   |\\_)__(_//|     Procs:    138
  <__)------(__>    Uptime:   42 mins
```

---

## 6. Milestone Checklist

Use the following interactive checklist to verify compliance with all project deliverables:

### Phase A: Kernel Baseline & Compilation
- [ ] Install all required dependencies (`build-essential`, `libncurses-dev`, `bison`, `flex`, `libssl-dev`, `libelf-dev`, `dwarves`).
- [ ] Obtain Linux v6.1 LTS source tree.
- [ ] Clear `CONFIG_SYSTEM_TRUSTED_KEYS` and `CONFIG_SYSTEM_REVOCATION_KEYS`.
- [ ] Set `CONFIG_LOCALVERSION="-os-A151064"` and disable `CONFIG_LOCALVERSION_AUTO`.
- [ ] Successfully compile kernel (`make -j$(nproc)`) and reboot.
- [ ] Verify active kernel via `uname -r` matching `6.1.*-os-A151064`.

### Phase B: Character Device Registration & Node Creation
- [ ] Dynamic device number assignment via `alloc_chrdev_region()`.
- [ ] Register `cdev` structure with `kfetch_fops`.
- [ ] Populate sysfs class (`class_create`) and device (`device_create`).
- [ ] Verify automatic node creation at `/dev/kfetch` with appropriate permissions.
- [ ] Implement proper rollback error handling on registration failures.

### Phase C: File Operations & User-Space Interaction
- [ ] Implement `kfetch_open()` and `kfetch_release()` with logging (`pr_info`).
- [ ] Implement `kfetch_write()` accepting 32-bit integer mask via `copy_from_user()`.
- [ ] Protect shared mask state with `mutex_lock()` and `mutex_unlock()`.
- [ ] Implement `kfetch_read()` returning formatted string via `copy_to_user()`.
- [ ] Implement EOF detection (`*f_pos > 0` returns `0`) preventing infinite read loops.

### Phase D: In-Kernel Telemetry Extraction
- [ ] Extract kernel release via `utsname()->release`.
- [ ] Extract hostname banner via `utsname()->nodename`.
- [ ] Extract CPU model name via `boot_cpu_data.x86_model_id` (with RISC-V portable fallback).
- [ ] Extract online core count via `num_online_cpus()`.
- [ ] Extract and convert memory via `si_meminfo()`.
- [ ] Extract uptime via `ktime_get_boottime_ts64()`.
- [ ] Count active tasks using `rcu_read_lock()` and `for_each_process()`.

### Phase E: Output Formatting & Mask Filtering
- [ ] Format 8-line ASCII Tux logo.
- [ ] Format variable-length separator dashes matching hostname length.
- [ ] Filter displayed fields strictly according to user-supplied bitmask.
- [ ] Maintain consistent left-column width (20 columns) regardless of mask selections.

### Phase F: Clean Uninstallation & Stability
- [ ] Clean module removal (`rmmod kfetch_mod_A151064`).
- [ ] Destroy device node (`device_destroy`), class (`class_destroy`), and cdev (`cdev_del`).
- [ ] Unregister device region (`unregister_chrdev_region`).
- [ ] Confirm no memory leaks, dangling pointers, or kernel panics in `dmesg`.

---

## 7. Screenshot Requirement Matrix

Students must capture and present verifiable terminal screenshots for each milestone. These screenshots must be included in `hw2_report.md` and the final submission PDF:

| ID | Test Item / Functional Stage | Execution Command | Required Visual Proof / Key Output | Report Section |
| :--- | :--- | :--- | :--- | :--- |
| **SS-01** | Kernel Version Verification | `uname -r` | Output shows `6.1.x-os-A151064` confirming custom kernel build | 2. Environment Setup |
| **SS-02** | Module Insertion & Node Creation | `sudo insmod kfetch_mod_A151064.ko && ls -l /dev/kfetch` | Shows `/dev/kfetch` exists with character device type (`c`) and Major/Minor numbers | 3. Module & Device Validation |
| **SS-03** | Kernel Ring Buffer Log | `dmesg \| tail -n 20` | Shows module init log, major/minor allocation, and device creation | 3. Module & Device Validation |
| **SS-04** | Default Telemetry Execution | `./kfetch` | Full Tux ASCII logo and all default telemetry lines side-by-side | 4. Functional Testing |
| **SS-05** | Filtered Mask Execution | `./kfetch -c -m` | Output filtered to only show CPU model and Memory stats alongside Tux | 4. Functional Testing |
| **SS-06** | Custom Multi-flag Execution | `./kfetch -r -u -p` | Output filtered to only show Kernel release, Uptime, and Process count | 4. Functional Testing |
| **SS-07** | Module Safe Unload | `sudo rmmod kfetch_mod_A151064 && dmesg \| tail -n 10` | Shows clean unregistration and `/dev/kfetch` removal | 5. Cleanup Verification |
