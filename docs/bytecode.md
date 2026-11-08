# Reading Umicom C bytecode

Bytecode is data describing operations for an interpreter. It is not an executable Windows or RISC-V machine-code file.

The file begins with eight bytes:

| Offset | Size | Meaning |
| --- | --- | --- |
| 0 | 4 | ASCII UMCB |
| 4 | 1 | Format version: 1 for arithmetic; 2 when comparisons occur |
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

All operands except PUSH's must be zero. Source offsets may not exceed the supported source limit. RETURN must be the last instruction and must find exactly one value. Every instruction is validated before any is executed.

For `7 * 6`, the stack is initially empty. PUSH 7 gives [7]. PUSH 6 gives [7, 6]. MULTIPLY gives [42]. RETURN reports 42.

## Profile compatibility

The encoder chooses the minimum required profile. A program containing only opcodes 1–7 retains its format-1 bytes exactly. A program with any comparison opcode 8–13 uses format 2. Both profiles retain the eight-byte header, nine-byte records, zero reserved flags and existing opcode values. Every comparison removes two operands and pushes one integer result.

The decoder accepts both profiles, validates the complete instruction sequence, then checks that the header matches the required profile. It rejects comparisons labelled as format 1 and arithmetic-only programs labelled as format 2. The latter rule gives each program one canonical encoding. Changing the version byte alone does not upgrade or downgrade a program. Older runtimes reject format 2 as an unknown version. Rebuild dependent consumers against the intended runtime and record its revision before qualifying them.

API structure version 1 remains unchanged. `UMC_COMPARISON_PROFILE` identifies headers that declare the comparison extension; it is not proof that a separately installed library matches those headers. The independent literal fixtures in [test_comparison_wire.c](../tests/test_comparison_wire.c) check both exact profiles.

## Portability

[bytecode.c](../src/bytecode.c) reads individual bytes. It does not cast an incoming buffer to a native structure. A structure can contain padding and use the processor's native byte order; neither belongs in a portable file.

The caller owns input and output buffers. If decoding fails, the output count is zero, even if some slots were written. Never execute those partial slots. Input and output must not overlap.

The format contains no addresses, jumps, system calls or external references. <!-- Earlier scope: It is deliberately limited to arithmetic. -->
It is limited to integer arithmetic and comparisons. Valid bytecode is not a proof of a useful or authorised application, and no signature is implied by a valid header.
