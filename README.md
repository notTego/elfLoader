# ELF Loader

The ELF Loader is a tool for loading ELF (Executable and Linkable Format) files into memory. It manages page faults and memory mapping to ensure that the program can execute correctly. This README provides an overview of the key components and resources used in the ELF Loader.

## Key Components

### Initial SIGSEGV Handle

The ELF Loader saves the initial SIGSEGV (Segmentation Violation) handler to manage page faults later in the loading process.

### Opening Source File Descriptor

It opens the source file's descriptor to read from it while mapping.

### SEGV_HANDLER

The SEGV_HANDLER component detects the faulty segment that caused a SIGSEGV signal using the address at which the signal was deployed. If this address is out of bounds for our known segments, the original handler is called.

### MAPPER

MAPPER is used as follows:

- It allocates a data array if necessary.
- It determines the page's start address and resolves possible permission issues (if the page has already been mapped in memory).
- It maps the page and updates the data array from the segment.
- It sets page protections.

## Resources Used

The ELF Loader utilizes the following resources for its operation:

1. [mmap(2) - Linux Manual Page](https://man7.org/linux/man-pages/man2/mmap.2.html)
   - Provides information about memory mapping.

2. [mprotect(2) - Linux Manual Page](https://man7.org/linux/man-pages/man2/mprotect.2.html)
   - Offers insights into changing memory protections.

3. [Stack Overflow](https://stackoverflow.com/questions/6015498/executing-default-signal-handler)
   - Provides guidance on executing the default signal handler.

The ELF Loader is a powerful tool for managing memory and loading ELF files into a running program. If you have any questions or need further assistance, please refer to the provided resources or seek help from the community.