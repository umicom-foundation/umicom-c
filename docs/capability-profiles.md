![Umicom](assets/branding/umicom-logo.svg)

# Ask the linked library what it supports

A header describes what your source can call. The library linked into the program supplies the implementation.
The new queries in [profile.h](../include/umicom_c/profile.h) report that implementation's declared features and bounds.
They do not authenticate a binary or replace source/toolchain receipts.

## First inspect the CLI

Build Umicom C using the [execution guide](execution-guide.md), then run the executable from that build:

```powershell
.\build\bin\umicom-c.exe profile
```

Use your actual build directory. The command takes no filename, reads no source and executes no bytecode.
Its output is a sequence of key=value lines with schema 1. Important fields are:

| Field | Meaning in this implementation |
| --- | --- |
<!-- Earlier contract retained for review: | compiler.features=65255 | arithmetic, comparisons, logical negation, literals, locals, logical connectives, conditional expressions, compound assignments, braced conditional statements, else-if chains, checked remainder and signed unary complement | -->
| compiler.features=130791 | arithmetic, comparisons, logical negation, literals, locals, logical connectives, conditional expressions, compound assignments, braced conditional statements, else-if chains, remainder, complement and binary bitwise operators |
<!-- Earlier contract retained for review: | runtime.features=16795 | arithmetic, comparisons, tracing, inspection, locals, forward branches and checked remainder | -->
| runtime.features=82331 | arithmetic, comparisons, tracing, inspection, locals, forward branches, remainder and binary bitwise operations |
<!-- Earlier contract retained for review: | runtime.wire-versions=62 | bits 1–5 are set: formats 1–5 | -->
| runtime.wire-versions=126 | bits 1–6 are set: formats 1–6 |
| compiler.local-values=32 / runtime.local-values=32 | per-program slot capacity |
| compiler.identifier-bytes=63 | source identifier length limit |
| compiler.source-bytes=65536 | maximum borrowed source span |
| compiler.instructions=256 | maximum emitted instruction count |
| runtime.stack-values=64 | maximum operand stack depth |

<!-- Earlier numeric mask retained for review: compiler.features=7 described arithmetic, comparison and logical not. The added source bits are 32 and 64; the runtime mask remains 27. -->

The numbers are bitmasks where documented. They are not feature counts. A format mask of 6 does not mean format 6.

## Query the actual component

```c
#include "umicom_c/profile.h"

uint32_t source_features = umc_compiler_profile(UMC_PROFILE_FEATURES);
if ((source_features & UMC_FEATURE_LOGICAL_NOT) != 0U) {
    /* A source editor may offer ! for the documented expression grammar. */
}

uint32_t runtime_features = umc_runtime_profile(UMC_PROFILE_FEATURES);
if ((runtime_features & UMC_FEATURE_EXECUTION_TRACE) != 0U) {
    /* The caller can prepare its own trace buffer before execution. */
}
```

Link the first call with UmicomC::Compiler; the second works with UmicomC::Runtime alone.
No output structure, allocation or mutable global state is involved. Existing public structures and wire encodings remain unchanged.

Logical negation belongs to the source compiler because it lowers to push-zero and equality.
The runtime does not advertise a logical-not opcode. Hexadecimal and binary spelling also belong to the compiler; they become ordinary integer operands. Test `UMC_FEATURE_HEXADECIMAL_LITERALS` or `UMC_FEATURE_BINARY_LITERALS` before offering those source forms in an editor. Conversely, the compiler query does not claim tracing or wire decoding as source grammar features.

## Bounds and unsupported queries

1. Query the component you actually intend to call.
2. Check the API value and required feature bits.
3. Compare reported limits with the capacities your program has allocated.
4. Keep checking every ordinary operation's status.
5. Keep source and library identities with your qualification evidence.

Unknown properties and properties not provided by that component return zero.
For example, runtime source length and compiler wire formats return zero.
A reported capacity does not allocate storage or prove that your input has that many readable bytes.

Older libraries without these additive functions fail to link; installing a new header alone cannot provide them.
If linkage fails, inspect the selected include directory and library path. Rebuild or select a matching SDK.
Do not silently remove the capability check to hide an unresolved library mismatch.

## Exercise the declarations

After building the tests, run:

```powershell
ctest --test-dir build -R "umicom.c.(profile|cli-profile)" --output-on-failure --no-tests=error
```

Expect three tests: compiler, runtime-only and CLI. The library tests exercise real source compilation,
wire encoding/decoding, inspection and traced execution. They also retain unsupported syntax and format refusals.

The optional Kernel CVM workload has a separate profile selector executed inside RISC-V user mode at O0 and O2.
Its receipt includes profile.h. A successful host CLI query does not substitute for that target-specific evidence.

A capability is a declared supported operation. It is not a complete-C23 claim, a memory-safety proof,
permission to execute arbitrary code, or evidence that a previously produced artifact came from this library.

## Local capability is shared across the boundary

`UMC_FEATURE_LOCAL_INTEGERS` is bit 128 in both components. The compiler recognizes declarations, assignments and names; the runtime validates and executes numeric slot operations. Query `UMC_PROFILE_LOCAL_VALUES` on either component, and `UMC_PROFILE_IDENTIFIER_BYTES` only on the compiler. The runtime returns zero for the latter because it does not parse identifiers.

<!-- Earlier profile table values retained for compatibility review: compiler.features=103, runtime.features=27, runtime.wire-versions=6. -->

<!-- Earlier profile wording retained for review:
The new masks are 231, 155 and 14 respectively.
-->
The local-only masks were 231, 155 and 14. <!-- Earlier profile wording retained for review:
With logical connectives and forward branches they are 743, 411 and 30.
-->
With logical connectives, forward branches and conditional expressions they were 1767, 411 and 30. Compound assignments added compiler bit 2048, producing masks 3815, 411 and 30. Braced conditional statements added compiler bit 4096, producing 7911. <!-- Earlier current-profile statement retained for review: Else-if chains add source bit 8192; the current masks are 16103, 411 and 30. -->
Else-if chains added source bit 8192, producing masks 16103, 411 and 30 before remainder. The current masks including remainder appear in the table above. Format 3 does not change legacy arithmetic or comparison artifact bytes. Use [local integer qualification](local-integers.md) alongside capability queries: a bit alone does not prove that an independently installed SDK was built from the intended source.

## Logical source and branch runtime are different capabilities

`UMC_FEATURE_LOGICAL_CONNECTIVES` (512) belongs to the compiler. `UMC_FEATURE_FORWARD_BRANCHES` (256) belongs to the runtime. The former recognizes `&&` and `||`; the latter admits and executes their lowered branches, without parsing source. Format-4 records require a matching runtime. Existing format-1, -2 and -3 programs retain their bytes.

## Conditional expressions reuse runtime control flow

`UMC_FEATURE_CONDITIONAL_EXPRESSIONS` is compiler bit 1024. It adds value-selecting source syntax through existing forward branches. The compiler mask was 743 before this addition and became 1767; the later compound-assignment addition made it 3815, braced conditional statements made it 7911, and else-if chains make it 16103. The runtime mask stays 411, and the accepted wire mask stays 30. Query individual capability bits rather than assuming a complete mask is frozen.

## Compound statements are source-only capability

`UMC_FEATURE_COMPOUND_ASSIGNMENTS` is compiler bit 2048. It recognizes adjacent `+=`, `-=`, `*=` and `/=` statements on initialized locals and lowers them through existing runtime instructions. The runtime must not advertise this bit. Public structure layouts, opcodes and wire masks do not change. Read [the update walkthrough](compound-assignments.md) and qualify both ordinary and failed updates.

## Braced conditional statement capability

**UMC_FEATURE_IF_STATEMENTS** is compiler bit 4096. It admits braced if/else updates with shared expression/statement depth, while reusing the runtime’s existing forward branches. The runtime does not advertise this source spelling. Test the bit and compile a representative conditional against the installed library before enabling the feature in a consumer. [The conditional guide](if-statements.md) describes the supported body grammar and qualification commands.

## Ordered source conditions

**UMC_FEATURE_ELSE_IF_CHAINS** is compiler bit 8192. It reports support for successive braced conditional bodies after **else if**. The runtime must not report this source-only bit: all resulting records use its existing forward branch capability. Check both the linked compiler feature and its independent runtime before enabling a source workflow. Header availability alone is not evidence that an older installed library implements the grammar.

<!-- Earlier capability rows retained for compatibility review:
| compiler.features=16103 | arithmetic, comparisons, logical negation, literals, locals, logical connectives, conditional expressions, compound assignments, braced conditional statements and else-if chains |
| runtime.features=411 | arithmetic, comparisons, tracing, inspection, locals and forward branches |
| runtime.wire-versions=30 | bits 1–4 are set: formats 1–4 |
-->

## Remainder across compiler and runtime

Both components advertise **UMC_FEATURE_REMAINDER**, bit 16384. Source compilation accepts % and %=; runtime-only consumers can validate and execute opcode 18 without linking the compiler. <!-- Earlier contract retained for review: The current complete masks are 65255 for the compiler, 16795 for the runtime and 62 for supported wire versions. -->
The current complete masks are 130791 for the compiler, 82331 for the runtime and 126 for supported wire versions. Query the individual bit in each component you actually use. Format 5 is selected only by a program containing remainder. A legacy program relabelled as format 5 is not canonical and is rejected. Existing arithmetic, comparison, local and branch encodings remain unchanged. See [checked remainder](remainder.md).

<!-- Earlier complete compiler mask statements retained for review before source complement:
| compiler.features=32487 | arithmetic, comparisons, logical negation, literals, locals, logical connectives, conditional expressions, compound assignments, braced conditional statements, else-if chains and checked remainder |
Both components advertise **UMC_FEATURE_REMAINDER**, bit 16384. Source compilation accepts % and %=; runtime-only consumers can validate and execute opcode 18 without linking the compiler. The current complete masks are 32487 for the compiler, 16795 for the runtime and 62 for supported wire versions. Query the individual bit in each component you actually use. Format 5 is selected only by a program containing remainder. A legacy program relabelled as format 5 is not canonical and is rejected. Existing arithmetic, comparison, local and branch encodings remain unchanged. See [checked remainder](remainder.md).
-->

## Unary complement is source lowering

**UMC_FEATURE_BITWISE_COMPLEMENT** is compiler bit 32768. It recognizes signed unary ~ through existing arithmetic. The runtime must not report this source bit. <!-- Earlier contract retained for review: Runtime features remain 16795 and wire versions remain 62. -->
Complement alone adds no runtime feature or wire version. The separate binary bitwise extension sets the current runtime features to 82331 and wire-version mask to 126. The [complement guide](complement.md) covers endpoint safety and independent consumers.

## Binary bitwise capability

Both the compiler and runtime advertise **UMC_FEATURE_BINARY_BITWISE**, bit 65536. The compiler admits &, ^, | and local compound statements &=, ^= and |=. Runtime-only consumers admit opcodes 19–21 and format 6 without linking source compilation. Check the feature in the component that will perform the work, and check bit 6 of wire_versions when transferring bytecode. A header macro is not a runtime capability check. See [binary bitwise operators](bitwise.md) for exact examples and refusal behavior.
