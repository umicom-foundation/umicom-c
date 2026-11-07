# Umicom C

An experimental C-family language, toolchain and C Virtual Machine for Umicom applications.

## Arithmetic compiler and bounded runtime

The first implementation accepts one integer-returning function, compiles it to portable bytecode and runs it with checked arithmetic and an instruction budget. It is not a complete C23 compiler. The library has no heap, file, process or network dependency.

Start with [your first program](docs/first-program.md), then read the [language](docs/language.md), [bytecode](docs/bytecode.md) and [embedding](docs/embedding.md) guides. The independent Kernel program and optional Framework adapter reuse this runtime.

Regression sources cover compilation, wire decoding, arithmetic, output preservation and operation comparison. Build and test results must be qualified for the selected host or target; source presence alone is not a runtime pass.

The [build and execution guide](docs/execution-guide.md) walks through a first
program, readable bytecode and the optional `--max-steps` execution budget.
It includes Windows commands, expected results and a controlled failure to inspect.

## Program inspection

Use `umicom-c inspect source.uc` or `umicom-c inspect-bytecode program.umcb` to examine instruction counts, stack requirements and portable bytecode size before execution. See [Inspect a program before execution](docs/program-inspection.md) for setup, API ownership, examples and limitations.
