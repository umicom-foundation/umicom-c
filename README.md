![Umicom](docs/assets/branding/umicom-logo.svg)

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

The current expression grammar also accepts `<`, `<=`, `>`, `>=`, `==` and `!=`. Arithmetic binds first, followed by relational comparisons, then equality. Results are signed integers 0 or 1. <!-- Earlier profile wording retained for review:
Both operands execute; this extension adds no logical short-circuiting, branches, variables or full C23 support.
-->
Both operands execute; this comparison extension adds no logical short-circuiting, branches or full C23 support. Local variables are provided by the separate extension below.

Existing arithmetic artifacts retain their exact format-1 encoding. Programs containing comparisons use format 2, which older runtimes reject. Read [the language grammar](docs/language.md) and [profile compatibility](docs/bytecode.md#profile-compatibility) before updating a consumer. The source, runtime, wire and CLI comparison groups supply independent expected values; Framework and Kernel have separate consumer checks. All must be executed against the selected source and toolchain before claiming qualification.

Unary logical negation is also supported: `!0` yields 1 and `!!-9` yields 1. It composes the existing comparison runtime and format-2 encoding. See [the language guide](docs/language.md#logical-negation) for precedence, instruction costs and failure behaviour.

## Linked capability profiles

Use umicom-c profile for a read-only summary of linked compiler and runtime capabilities. Embedders can query the same declarations through umicom_c/profile.h. See [capability profiles](docs/capability-profiles.md) for feature masks, runtime-only linking and SDK identity limits.

## Numeric source spellings

[Write and check numeric literals](docs/integer-literals.md) introduces decimal, hexadecimal and binary constants in the bounded signed integer language. The existing runtime operations and wire formats are unchanged; octal, suffixes and unsigned types remain outside the profile.

## Configuration input evidence

Standalone configuration records declared source, generated configuration and tool identities. The [input evidence guide](docs/configuration-inputs.md) shows a read-only stale check and explains why a matching record does not certify the compiled binary or replace tests. Runtime-only embedding remains independent.

## Compare source operations

The read-only `compare-operations left.uc right.uc` command reuses the validated instruction-identity service. It reports sameness separately from differences and invalid input, without executing a program. Follow [the comparison walkthrough](docs/operation-comparison.md) before using its exit codes in a workflow.

## Initialized local integers

Programs can declare up to 32 initialized integer locals, assign to existing names and read those values in expressions. The shared VM checks slot bounds and definite initialization before execution. Programs with local operations use canonical wire format 3; earlier artifact encodings remain stable. Follow [local integers](docs/local-integers.md) for an eight-instruction example, source and runtime restrictions, and consumer qualification.

## Guard expressions with logical operators

`&&` and `||` evaluate left to right and skip an unnecessary right operand. Their forward-only branches use canonical format 4. The runtime checks stack joins and local initialization over every structural path before executing one path. [The short-circuit guide](docs/short-circuit.md) demonstrates guarded division, actual fuel cost, trace gaps and native-C comparison tests. This does not add loops or arbitrary full-C control flow.

## Select an integer value

Use `condition ? when_true : when_false` to evaluate one of two integer expressions. The selected value is preserved, and nested conditionals associate to the right. This source extension reuses the existing forward branch runtime and format 4. [Conditional values](docs/conditional-values.md) covers fallback arithmetic, recursion limits, exact wire fixtures and consumer qualification.
