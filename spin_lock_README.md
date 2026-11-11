# Linux Out-of-Tree Character Device Driver (`demodev`)

A robust, production-grade Linux character device driver developed for ARM Cortex-A8 (BeagleBone Black) running Debian, designed from first principles for Canonical Linux Kernel Engineering standards.

CLI terminal test-multi-thread-stress result:
![Result](jpeg/test-multi-thread-stress.png)

multi-threading-kernel-driver
![Result](jpeg/multi-threading-kernel-driver.jpeg)

---
# Linux Out-of-Tree Character Device Driver (`demodev`)

A robust, production-grade Linux character device driver developed for ARM Cortex-A8 (BeagleBone Black) running Debian, designed from first principles for Canonical Linux Kernel Engineering standards.

---

## Architecture & Features

- **Dynamic Device Registration:** Automatically allocates a major number via `register_chrdev()` and creates device nodes in `/dev/demodev` via modern kernel `class_create()` and `device_create()` APIs.
- **Persistent Kernel Buffer:** Maintains a 1024-byte kernel storage buffer (`global_kernel_buf`) supporting cross-file descriptor interactions (e.g., separate `echo`/`tee` and `cat` invocations).
- **SMP Concurrency Protection:** Employs kernel spinlocks (`spinlock_t`) to serialize critical sections and protect shared global states against multi-core SMP race conditions, separating sleeping context operations from locked regions.
- **ioctl Command & Telemetry Interface:** Supports custom `ioctl` commands (`.unlocked_ioctl`) using magic number `'k'`:
  - `DEMODEV_IOC_RESET`: Securely resets and zeroes out the kernel buffer.
  - `DEMODEV_IOC_GET_STATS`: Queries real-time telemetry structure (`demodev_stats`) containing total write count, read count, and current buffer length.
- **Multi-Threaded Stress Test Suite (`stress_app`):** Validates driver thread-safety and lock contention under heavy concurrent load using POSIX threads (`pthread`).
- **Safe Boundary Copying:** Implements secure kernel-to-userspace memory transfer using `copy_to_user()` and `copy_from_user()` with rigorous bounds checking and error handling (`-EFAULT`, `-ENOMEM`).
- **Unified Build System:** A streamlined `Makefile` that compiles the kernel module (`demodev.ko`), standard userspace test app (`app`), and multi-threaded stress test (`stress_app`).

---

## Repository Structure

```text
.
├── demodev.c       # Core kernel module source (persistent character driver with spinlocks & ioctl)
├── app.c           # Userspace test application exercising I/O and ioctl commands
├── stress_app.c    # Multi-threaded POSIX pthreads concurrency stress test application
├── Makefile        # Unified build infrastructure for kernel module and binaries
├── README.md       # Project documentation and build/test instructions
└── jpeg/           # Architecture diagrams and hardware verification photos

Build & Installation Instructions
1. Compile the Module and Application
Run make to compile both the kernel module and the userspace test program:

Bash
make clean
make
2. Load the Kernel Module
Insert the compiled module into the running kernel and verify initialization:

Bash
sudo insmod demodev.ko
dmesg | tail -n 5
ls -l /dev/demodev

3. Test I/O Operations
You can test the driver using standard shell utilities or the companion test application:

Using Shell Utilities:

Bash
echo "Canonical Linux Kernel Engineering" | sudo tee /dev/demodev
sudo cat /dev/demodev
Using the Companion App:

Bash
sudo ./app

sudo ./stress_app

4. Unload the Module
Remove the module when testing is complete:

Bash
sudo rmmod demodev
dmesg | tail -n 3

Author
Natsagnyam Namkhai

Target Role: Linux Kernel Engineer, Canonical


