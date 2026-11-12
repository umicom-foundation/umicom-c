# Reading Umicom C bytecode

Bytecode is data describing operations for an interpreter. It is not an executable Windows or RISC-V machine-code file.

The file begins with eight bytes:

| Offset | Size | Meaning |
| --- | --- | --- |
| 0 | 4 | ASCII UMCB |
| 4 | 1 | Minimum profile: 1 arithmetic, 2 comparisons, 3 local slots, 4 forward branches |
<!-- Earlier format row: 1 for arithmetic; 2 when comparisons occur. -->
<!-- Original format row: | 4 | 1 | Format version: 1 | -->
| 5 | 1 | Reserved: must be zero |
| 6 | 2 | Instruction count, least significant byte first |

Each instruction occupies nine bytes:

| Offset within instruction | Size | Meaning |
| --- | --- | --- |
| 0 | 1 | Opcode |
| 1 | 4 | Signed 32-bit operand, two's-complement, least significant byte first |
| 5 | 4 | Source byte offset |

The exact length is `8 + count * 9`. A decoder rejects truncated records, trailing bytes, unknown versions, nonzero reserved flags and invalid instruction shapes.

## Operations

| Number | Name | Stack effect |
| --- | --- | --- |
| 1 | PUSH | Add its integer operand |
| 2 | ADD | Remove two values and push their sum |
| 3 | SUBTRACT | Remove right then left; push left minus right |
| 4 | MULTIPLY | Remove two values and push their product |
| 5 | DIVIDE | Remove right then left; push left divided by right |
| 6 | NEGATE | Replace the last value with its negative |
| 7 | RETURN | Complete with the sole remaining value |
| 8 | LESS | Remove right then left; push 1 if left < right, otherwise 0 |
| 9 | LESS_EQUAL | Push 1 if left <= right, otherwise 0 |
| 10 | GREATER | Push 1 if left > right, otherwise 0 |
| 11 | GREATER_EQUAL | Push 1 if left >= right, otherwise 0 |
| 12 | EQUAL | Push 1 if left == right, otherwise 0 |
| 13 | NOT_EQUAL | Push 1 if left != right, otherwise 0 |
| 14 | LOAD_LOCAL | Push the initialized value of slot operand 0–31 |
| 15 | STORE_LOCAL | Pop one value into slot operand 0–31 |
| 16 | JUMP | Continue at absolute instruction index operand; stack unchanged |
| 17 | JUMP_IF_ZERO | Pop one value; jump to operand if zero, otherwise continue |

<!-- Earlier profile wording retained for review:
All operands except PUSH's must be zero.
-->
<!-- Earlier profile wording retained for review:
PUSH carries an integer constant. LOAD_LOCAL and STORE_LOCAL carry a slot index from 0 through 31. Every other operand must be zero. A load requires a preceding store to that same slot in this straight-line program. Source offsets may not exceed the supported source limit. RETURN must be the last instruction and must find exactly one value. Every instruction is validated before any is executed.
-->
PUSH carries an integer constant. Local operands are slot indices 0–31. Branch operands are absolute record indices greater than the current index and less than count. Every other operand is zero. Each local read requires a store on every incoming path. Joined paths must have equal stack depth. Every record must be structurally reachable from the entry. Source offsets are bounded, and RETURN must be last with exactly one value. Complete admission precedes execution.

For `7 * 6`, the stack is initially empty. PUSH 7 gives [7]. PUSH 6 gives [7, 6]. MULTIPLY gives [42]. RETURN reports 42.

## Profile compatibility

The encoder chooses the minimum required profile. A program containing only opcodes 1–7 retains its format-1 bytes exactly. <!-- Earlier profile wording retained for review:
A program with any comparison opcode 8–13 uses format 2.
-->
<!-- Earlier profile wording retained for review:
A program with comparisons and no local operations uses format 2. Any LOAD_LOCAL or STORE_LOCAL requires format 3, even when a comparison occurs earlier.
-->
Comparisons without locals or branches use format 2. Locals without branches use format 3. Any JUMP or JUMP_IF_ZERO requires format 4, regardless of the other records. <!-- Earlier profile wording retained for review:
Both profiles retain the eight-byte header, nine-byte records, zero reserved flags and existing opcode values.
-->
<!-- Earlier profile wording retained for review:
All three profiles retain the eight-byte header, nine-byte records, zero reserved flags and existing opcode values.
-->
All four profiles retain the eight-byte header, nine-byte records, zero reserved flags and existing opcode values. Every comparison removes two operands and pushes one integer result.

<!-- Earlier profile wording retained for review:
The decoder accepts both profiles, validates the complete instruction sequence, then checks that the header matches the required profile.
-->
<!-- Earlier profile wording retained for review:
The decoder accepts profiles 1, 2 and 3, validates the complete instruction sequence, then checks that the header matches the minimum required profile.
-->
The decoder accepts profiles 1–4, validates the complete instruction graph, then checks that the header matches the minimum required profile. It rejects comparisons labelled as format 1 and arithmetic-only programs labelled as format 2. The latter rule gives each program one canonical encoding. Changing the version byte alone does not upgrade or downgrade a program. Older runtimes reject format 2 as an unknown version. Rebuild dependent consumers against the intended runtime and record its revision before qualifying them.

API structure version 1 remains unchanged. `UMC_COMPARISON_PROFILE` identifies headers that declare the comparison extension; it is not proof that a separately installed library matches those headers. The independent literal fixtures in [test_comparison_wire.c](../tests/test_comparison_wire.c) check both exact profiles.

## Portability

[bytecode.c](../src/bytecode.c) reads individual bytes. It does not cast an incoming buffer to a native structure. A structure can contain padding and use the processor's native byte order; neither belongs in a portable file.

The caller owns input and output buffers. If decoding fails, the output count is zero, even if some slots were written. Never execute those partial slots. Input and output must not overlap.

<!-- Earlier profile wording retained for review:
The format contains no addresses, jumps, system calls or external references.
-->
The format contains no native addresses, backward jumps, system calls or external references. <!-- Earlier scope: It is deliberately limited to arithmetic. -->
<!-- Earlier profile wording retained for review:
It is limited to integer arithmetic and comparisons.
-->
<!-- Earlier profile wording retained for review:
It is limited to integer arithmetic, comparisons and bounded local storage.
-->
It is limited to integer arithmetic, comparisons, bounded local storage and forward control flow. Valid bytecode is not a proof of a useful or authorised application, and no signature is implied by a valid header.

## Local storage admission

The validator uses a 32-bit initialization map. It first checks that the signed slot operand is nonnegative and below 32; only then may it shift a bit. STORE requires one stack value and marks the slot initialized. LOAD requires that bit and one free stack position. The executor receives only admitted instructions and uses its own automatic 32-element integer array. There is no cross-call local state and no native pointer in a record.

[test_local_runtime.c](../tests/test_local_runtime.c) includes literal format-3 bytes, invalid indices, read-before-write, fuel exhaustion and consecutive invocation checks without linking the compiler.

## Join validation

Forward targets let the validator process records in index order: every predecessor is visited before its target. The first incoming edge records stack depth and initialized-local bits. Later edges must have the same depth and intersect their initialized bits with the recorded set. A local initialized on only one arm is therefore unavailable after the join. Both conditional edges count, even for a constant condition. Unreachable records are rejected instead of hiding unchecked data.

Inspection counts all records and reports the maximum stack over structural paths. Execution counts only the path taken. Since each step advances the index, count is a sufficient fuel upper bound and trace capacity bound. The independent [forward branch tests](../tests/test_forward_branches.c) exercise literal format-4 bytes and malformed joins without linking a compiler.
