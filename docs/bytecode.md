![Umicom](assets/branding/umicom-logo.svg)

# Reading Umicom C bytecode

Bytecode is data describing operations for an interpreter. It is not an executable Windows or RISC-V machine-code file.

The file begins with eight bytes:

| Offset | Size | Meaning |
| --- | --- | --- |
| 0 | 4 | ASCII UMCB |
<!-- Earlier contract retained for review: | 4 | 1 | Minimum profile: 1 arithmetic, 2 comparisons, 3 local slots, 4 forward branches, 5 remainder | -->
| 4 | 1 | Minimum profile: 1 arithmetic, 2 comparisons, 3 local slots, 4 forward branches, 5 remainder, 6 binary bitwise |
<!-- Earlier profile row retained for review: | 4 | 1 | Minimum profile: 1 arithmetic, 2 comparisons, 3 local slots, 4 forward branches | -->
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
| 18 | REMAINDER | Remove right then left; push signed left % right after divisor and range checks |
| 19 | BITWISE_AND | Remove two values; push signed 32-bit AND |
| 20 | BITWISE_XOR | Remove two values; push signed 32-bit XOR |
| 21 | BITWISE_OR | Remove two values; push signed 32-bit OR |

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
<!-- Earlier profile wording retained for review:
Comparisons without locals or branches use format 2. Locals without branches use format 3. Any JUMP or JUMP_IF_ZERO requires format 4, regardless of the other records.
-->
<!-- Earlier contract retained for review: Without remainder, comparisons without locals or branches use format 2, locals without branches use format 3, and any JUMP or JUMP_IF_ZERO requires format 4. Any REMAINDER requires format 5, including when a branch occurs earlier in the record sequence. The encoder selects the maximum profile requirement over the complete program. -->
Without remainder or binary bitwise operations, comparisons require format 2, locals require format 3 and forward branches require format 4. REMAINDER requires format 5. Any BITWISE_AND, BITWISE_XOR or BITWISE_OR requires format 6. The encoder selects the maximum requirement across the complete program, even when remainder occurs before a bitwise operation. <!-- Earlier profile wording retained for review:
Both profiles retain the eight-byte header, nine-byte records, zero reserved flags and existing opcode values.
-->
<!-- Earlier profile wording retained for review:
All three profiles retain the eight-byte header, nine-byte records, zero reserved flags and existing opcode values.
-->
<!-- Earlier profile wording retained for review:
All four profiles retain the eight-byte header, nine-byte records, zero reserved flags and existing opcode values.
-->
<!-- Earlier contract retained for review: All five profiles retain -->
All six profiles retain the eight-byte header, nine-byte records, zero reserved flags and existing opcode values. Every comparison removes two operands and pushes one integer result.

<!-- Earlier profile wording retained for review:
The decoder accepts both profiles, validates the complete instruction sequence, then checks that the header matches the required profile.
-->
<!-- Earlier profile wording retained for review:
The decoder accepts profiles 1, 2 and 3, validates the complete instruction sequence, then checks that the header matches the minimum required profile.
-->
<!-- Earlier profile wording retained for review:
The decoder accepts profiles 1–4, validates the complete instruction graph, then checks that the header matches the minimum required profile.
-->
<!-- Earlier contract retained for review: The decoder accepts profiles 1–5, -->
The decoder accepts profiles 1–6, validates the complete instruction graph, then checks that the header matches the minimum required profile. It rejects comparisons labelled as format 1 and arithmetic-only programs labelled as format 2. The latter rule gives each program one canonical encoding. Changing the version byte alone does not upgrade or downgrade a program. Older runtimes reject format 2 as an unknown version. Rebuild dependent consumers against the intended runtime and record its revision before qualifying them.

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

## Remainder admission and execution

REMAINDER has a zero reserved operand and consumes two stack values. Its source offset points to the % or %= operator in compiler-produced programs. Complete graph admission occurs before execution. At runtime, a zero divisor or the minimum signed integer paired with -1 fails before evaluating native C remainder. Caller result storage remains unchanged; a trace retains the failing instruction and its two input values. Fuel exhaustion also occurs before the operation.

The independent format-5 fixture in [test_remainder.c](../tests/test_remainder.c) is literal bytes, not output produced by the encoder under test. It checks exact encoding, decoding, execution, source offsets and profile refusal. Legacy arithmetic relabelled as format 5 and remainder relabelled as format 4 are both rejected. See [checked remainder](remainder.md) for the complete bytes and commands.

## Binary bitwise admission

Opcodes 19 through 21 require zero reserved operands and two input values. Their result preserves a defined signed 32-bit representation. They consume one instruction of fuel each; a failed admission or exhausted budget cannot publish a completed value. Inspection retains the existing meaning of arithmetic_count, so these operations do not increase that field. Literal format-6 bytes in [test_bitwise.c](../tests/test_bitwise.c) check encoding independently of the encoder, including wrong versions, reserved operands and signed endpoints. See [binary bitwise operators](bitwise.md) for source precedence and consumer profiles.

## Checked shifts and format 7

SHIFT_LEFT=22 and SHIFT_RIGHT=23 append stable identities with zero reserved operands. Each consumes two values and produces one. A program containing either requires canonical format 7; a program without them retains its previous minimum format and bytes. Select the greatest requirement over the entire program, including bitwise records before shifts. Runtime count/domain checks preserve the failed instruction and final-result output. See [checked shifts](checked-shifts.md) for the nonnegative domain and independent transport cases.
