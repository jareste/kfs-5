# KFS - A Custom Kernel Project

![Language](https://img.shields.io/badge/language-C/C++/Assembly-blue)
![Platform](https://img.shields.io/badge/platform-x86_64-blue)

KFS is a custom kernel project built from scratch with a focus on low-level system programming and operating system concepts. This project serves as an educational endeavor to deepen my understanding of kernel development, memory management, multitasking, and hardware interactions. Some parts may not follow best practices strictly, as they were implemented with a focus on learning and exploration.

---

## 🚀 Features

- **Custom Bootloader:** Loads the kernel and transfers control efficiently.
- **Interrupt Handling:** Supports both hardware and software interrupts.
- **Memory Management:** Implements custom malloc, free, and paging.
- **Multitasking:** Supports task scheduling and context switching in a non-preemptive way.
- **Filesystem (ext2):** Basic implementation of the ext2 filesystem.
- **Drivers:** 
  - Keyboard input.
  - Screen output.
  - Basic disk IO.
  - More drivers under development.
- **Syscalls:** Basic syscalls implemented via interrupts.
- **User-space (WIP):** Supports running tasks as a user. Currently, function mapping to user-space causes page faults. Contributions or suggestions are welcome!

---

## 🛠️ Building and Running

### Requirements

- **GCC** or **Clang** (for cross-compilation)
- **NASM** (for assembly code)
- **QEMU** (for testing the kernel)

### Build Instructions

1. **Clone the repository:**
   ```bash
   git clone https://github.com/jareste/kfs.git
   cd kfs

2. **Build the kernel:**
   ```bash
   make

3. **Create the disk image:**
   ```bash
   make format

4. **Run with QEMU:**
   ```bash
   make run

## 🛠️ Tools and languages

- **C:** Core language for kernel development.
- **ASM:** Used for bootloader and low-level hardware interactions.
- **Make:** Build automation.
- **QEMU:** Testing, running and debugging.

## 🧩 WIP & Roadmap

- **User-space:** Implement a proper user-space environment.
- **Modules:** Create an API for driver registration and management.
- **ELF parser:** Enable parsing and execution of binary files (mandatory for user-space).
- **Memory monitor:** Implement a monitor to detect memory corruption and prevent undefined behaviors.
- **Shell:** Create a basic UNIX-like shell.
- **Readme:** Expand this README with key design decisions and rationale.

## 📚 Documentation

I mainly followed [OSDev](https://wiki.osdev.org/Expanded_Main_Page) docs

## 📜 License

This project is currently not licensed as it is developed purely for learning purposes. However, if you are interested in using or contributing to it, please reach out!
