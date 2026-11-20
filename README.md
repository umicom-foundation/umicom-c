![Umicom](docs/assets/branding/umicom-logo.svg)

# Umicom C

An experimental C-family language, toolchain and C Virtual Machine for Umicom applications.

## Arithmetic compiler and bounded runtime

The first implementation accepts one integer-returning function, compiles it to portable bytecode and runs it with checked arithmetic and an instruction budget. It is not a complete C23 compiler. The library has no heap, file, process or network dependency.

Start with [your first program](docs/first-program.md), then read the [language](docs/language.md), [bytecode](docs/bytecode.md) and [embedding](docs/embedding.md) guides. The independent Kernel program and optional Framework adapter reuse this runtime.

The [browser integer runner](docs/browser-arithmetic.md) opens the same
format-1 artifact in a standard browser worker and offers opt-in format-2
comparisons, [format-3 local integers](docs/browser-locals.md) and
[format-4 forward branches](docs/browser-branches.md). The optional
[integer profile](docs/browser-integers.md) also admits formats 5 through 7 for
remainder, bitwise operators and checked shifts. The worker validates the
complete program before evaluating its signed 32-bit instructions.

The [required-input entry](docs/int-input.md) lets the native CLI and a browser
worker bind different signed integers to the same compiled artifact. It uses
the existing checked local variables, instructions and execution budget.

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

## Update an existing local value

Use `+=`, `-=`, `*=` and `/=` as statements on initialized integer locals. The compiler reuses the runtime's checked arithmetic and local operations, preserving canonical formats 3 and 4. Follow [compound assignments](docs/compound-assignments.md) for the eight-instruction example, overflow diagnosis and native-C comparison coverage. <!-- Earlier limitation retained for review: Assignment expressions, remainder and bitwise updates remain unsupported. -->
<!-- Earlier limitation retained: Assignment expressions and bitwise updates remain unsupported. Checked remainder and its compound update are provided by the separate extension below. -->
Assignment expressions remain unsupported. Checked remainder and bitwise compound updates are provided by the extensions below.

## Choose a group of updates

<!-- Earlier scope restriction retained for engineering history:
Braced **if** and optional **else** bodies can update initialized locals or contain nested braced conditionals. Conditions run once, only the selected body executes, and both bodies are checked during compilation. The final return remains required; body-local declarations and early returns remain unsupported. Follow [conditional statements](docs/if-statements.md) for exact paths, combined recursion bounds and format-4 qualification.
-->
Braced **if** and optional **else** bodies can declare initialized locals, update visible locals or contain nested braced conditionals. A body-local name becomes unavailable after its closing brace. Conditions run once, only the selected body executes, and both bodies are checked during compilation. The final return remains required; early returns and shadowing a currently visible name remain unsupported. Follow [conditional statements](docs/if-statements.md) and [branch-local scope](docs/branch-local-scope.md) for paths, bounds and format-4 behavior.

## Ordered conditional chains

The compiler accepts braced **else if** chains through the existing forward-branch runtime. Conditions are tested in order; only the first selected update body executes. Read [ordered else-if chains](docs/else-if-chains.md) for the complete source example, profile bit, exact fuel and remaining grammar limits.

## Checked integer remainder

Use **%** to calculate a signed remainder or **%=** to update an initialized local. The runtime refuses zero divisors and the minimum-integer/-1 pair without publishing a result. The [remainder walkthrough](docs/remainder.md) explains sign rules, exact format-5 bytes, feature queries, bounded execution and the shared Kernel/Framework consumer boundary. Earlier artifacts keep their existing encodings.

## Signed unary complement

Unary **~** flips a signed 32-bit value using existing checked arithmetic. Both signed endpoints work without a new bytecode version. Read [signed complement](docs/complement.md) for a five-instruction example, exact fuel, capability queries and consumer checks. <!-- Earlier unary-only limit retained for review: Binary bitwise operators, shifts and unsigned types remain outside the language profile. -->
<!-- Earlier bitwise boundary retained for review: Binary bitwise operators are supported by the separate extension; shifts and unsigned types remain outside the language profile. -->
Read the [binary bitwise](docs/bitwise.md) and [checked shift](docs/checked-shifts.md)
guides for the signed 32-bit operations and their bounds.

## Standalone updates by one

Use `++value;`, `value++;`, `--value;` or `value--;` on initialized integer locals, including inside conditional bodies. These statements reuse checked arithmetic and preserve existing runtime and wire contracts. [Change an integer by one](docs/increment-statements.md) explains the statement-only grammar, exact execution budget and overflow handling.

## Early feedback and paused execution

Use [the seven-lesson execution course](docs/execution-course.html) to learn syntax checking, stepping, bytecode and explicit-root collection. [Interactive debugging](docs/interactive-debugging.md) documents debug and debug-bytecode commands. [Managed objects](docs/managed-objects.md) describes the optional separate handle heap. [Language regions](docs/language-regions.md) documents classification-only tags; BASIC, Java and full C23 compilation remain outside this implementation.

## BASIC statement-comment boundaries

The region-discovery command now protects delimiters inside case-insensitive
`REM` comments at a BASIC line or colon statement start. The documented keyword
boundary prevents `Remember` from becoming a comment. This changes lexical
classification only; BASIC execution and mixed-language compilation remain
unsupported. See [tagged language regions](docs/language-regions.md) and
[the complete example](examples/basic_comments.umc).

Use `const int` or `int const` for an initialized local that source code must
not update. The compiler rejects writes before bytecode execution; see
[read-only local integers](docs/readonly-locals.md) for a worked example and
the distinction between a source qualifier and runtime storage.

## Temporary calculation scopes

A standalone braced block can contain initialized temporary locals, updates and
nested blocks or conditions. Names end at their matching closing brace; active
outer names cannot be shadowed. Read [compound statements](docs/compound-statements.md)
for an eight-instruction source/bytecode example, exact fuel, error recovery and
the distinction between compiler scope and runtime storage. No new VM opcode is needed.

## Optional source warnings

Use `check source.uc --warnings` to report initialized locals whose values are never read in the admitted source. The native `umc_check_source` service shares the existing parser, preserves the caller’s report on failure, and does not execute arithmetic. Read [source warnings](docs/source-warnings.md) for byte positions, scope reuse, exit statuses and the distinction from dead-store analysis.

## Structural instruction budgets

Use budget source.uc or budget-bytecode program.umcb to count the shortest and longest structural paths without executing arithmetic. The shared runtime validates the graph first and considers both conditional alternatives. Read [instruction budgets](docs/path-budgets.md) for exact commands, a four-to-seven-step example and the limits of the estimate.


## Boolean literal spellings

Use exact lowercase true and false for integer values 1 and 0 in the bounded expression language. The [Boolean literal walkthrough](docs/boolean-literals.md) explains short-circuit behavior, reserved names, existing wire formats and the distinction from typed bool storage. This adds a compiler feature without changing the VM.

## C and Java translation

The compiler can translate one checked integer return function between C
and Java. See [the translation exercise](docs/c-and-java-translation.md) for
the supported declarations, round-trip commands and overflow check.
