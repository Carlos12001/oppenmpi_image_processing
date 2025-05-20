## Distributed Image Processing System with Filters and Hardware Visualization – Raspberry Pi 3

This project implements a distributed image processing system using a **Raspberry Pi 3**, combining parallel filtering algorithms, network communication, hardware-based visualization, and classic encryption for data security. The system applies filters such as **Sobel**, **Blur**, **Grayscale**, and **RGB enhancement** (Red, Green, Blue), and computes the **Frobenius norm** before and after processing. The architecture integrates C-based software with **OpenMPI**, a custom **kernel-space driver**, and a **real-time GPIO I/O interface** for hardware interaction.

### 🚀 Key Features

- 🔧 **Custom Linux Kernel Driver in C**  
  The system includes a character driver (`gpio_driver.c`) that enables:
  - Writing 3-digit numbers to 7-segment displays via GPIO.
  - Reading a 3-bit binary code from GPIO input pins, which selects the filter to apply.
  - Modular and efficient control of multiple displays and input lines from user space.

- 🖼️ **Distributed Image Processing with OpenMPI**  
  Using MPI, the BMP image is split into horizontal segments and processed in parallel:
  - Available filters: Sobel, Blur, Grayscale, Red, Green, Blue.
  - Results are gathered and the Frobenius norm is computed in a distributed fashion.

- 🔐 **Caesar Cipher for BMP Image Encryption**  
  Prior to distributing data, a custom Caesar-based encryption with per-byte shift is applied to ensure image integrity during transmission.

- 📤 **Client-Server Image Transfer**  
  Images are transferred using TCP/IP from a client to the processing server, avoiding shared file dependencies.

- 📺 **Embedded GPIO Interface**  
  - Final Frobenius norm is displayed on the 7-segment hardware.
  - User can interact with the system using physical buttons to choose the desired filter.
