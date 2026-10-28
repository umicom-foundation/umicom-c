# Embed the compiler and runtime

An SDK is a library, its public declarations and the metadata needed to link another program to it. Umicom C exports two CMake targets:

- `UmicomC::Runtime`: diagnostics, bytecode, operation comparison and execution.
- `UmicomC::Compiler`: the source lexer and parser, linked to Runtime.

The [installed consumer](../examples/sdk_consumer/main.c) compiles a small source span and executes it. Its [CMake project](../examples/sdk_consumer/CMakeLists.txt) uses `find_package(UmicomC 0.1 CONFIG REQUIRED)`. It does not compile another copy of the SDK.

## Ownership

Allocate an instruction array and pass its capacity. Supply the source pointer together with its byte length; a terminating zero is not required. Keep all buffers valid and disjoint throughout the call.

Check the returned status before reading a result. Compilation and decoding set their output counts to zero on failure. Execution leaves the supplied result unchanged on failure. A nonzero value left from a previous successful run is therefore not evidence of a new success.

Diagnostic message pointers refer to static storage. Do not free them. A diagnostic source offset identifies a byte in the supplied source, not a file opened by the library.

The runtime has no allocation, file, network, process, host-call or executable-memory interface. Concurrent users need separate writable buffers. The instruction array must remain immutable during execution.

## Instruction budgets

An instruction budget, also called fuel, limits how many operations may execute. RETURN consumes one instruction. A four-instruction program needs a budget of at least four.

Validation still examines the whole bounded program before execution. Fuel does not limit validation time, and is not a wall-clock deadline. A host that needs deadlines must supply them around the call or through process isolation.

## Comment changes

`umc_program_same_operations`, declared in [comparison.h](../include/umicom_c/comparison.h), compares validated opcodes and operands while ignoring source offsets. This identifies equal operation streams after comment or whitespace changes.

It does not prove equivalence of different algorithms and does not authorize reusing test results or build output. Changed source locations may require new diagnostics; compiler versions, flags, external inputs and dependencies still matter.

## Hosted and freestanding consumers

A hosted program runs under an operating system and can use facilities such as standard file I/O. The CLI is hosted. The runtime itself can be compiled without those facilities through [UmicomCRuntime.cmake](../cmake/UmicomCRuntime.cmake).

Umicom Kernel's optional CVM program compiles the runtime for RISC-V and uses the Kernel's native EXIT call. The host executable cannot be copied into that address space. The Linux-based Umicom OS probe is another, separate target-native consumer. Neither arrangement makes arbitrary C programs portable.

Umicom Framework owns its optional source-preview service. Applications can consume the same result model instead of implementing their own compiler error handling. That service is a source-composed optional adapter; the main Framework SDK does not automatically enable it.
