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

## Execution traces

Use `umicom-c trace source.uc` or `umicom-c trace-bytecode program.umcb` to observe the shared arithmetic executor. Rows describe successful operations and a rejected runtime step; malformed input and insufficient trace storage execute nothing. Read [Observe a bounded execution trace](docs/execution-traces.md) for command examples, caller-owned buffers, fuel and output-preservation rules. <!-- Trace-only addition originally stated: The bytecode format and existing execution API remain unchanged. -->
The trace API keeps the same storage contract. The comparison extension described below selects an additional wire profile.

## Integer comparison expressions

The current expression grammar also accepts `<`, `<=`, `>`, `>=`, `==` and `!=`. Arithmetic binds first, followed by relational comparisons, then equality. Results are signed integers 0 or 1. Both operands execute; this extension adds no logical short-circuiting, branches, variables or full C23 support.

Existing arithmetic artifacts retain their exact format-1 encoding. Programs containing comparisons use format 2, which older runtimes reject. Read [the language grammar](docs/language.md) and [profile compatibility](docs/bytecode.md#profile-compatibility) before updating a consumer. The source, runtime, wire and CLI comparison groups supply independent expected values; Framework and Kernel have separate consumer checks. All must be executed against the selected source and toolchain before claiming qualification.
