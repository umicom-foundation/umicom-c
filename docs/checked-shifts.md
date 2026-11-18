![Umicom](assets/branding/umicom-logo.svg)

# Checked integer shifts

A bit is one binary digit, either zero or one. A nonnegative integer can be written as a row of bits. Moving that row one position left multiplies its value by two. Moving it one position right divides by two and discards the fractional part.

Umicom C supports `<<`, `>>`, and the local update statements `<<=` and `>>=`. These operations are a deliberately checked subset: the left input must be nonnegative, the count must be from 0 to 31, and the result must fit in a signed 32-bit integer. Invalid values return `UMC_ARITHMETIC_ERROR` when the instruction executes. A valid source check does not prove that runtime arithmetic will succeed.

| Source | Result | Reason |
| --- | --- | --- |
| `21 << 1` | 42 | Multiply 21 by two |
| `85 >> 1` | 42 | Divide 85 by two and discard the half |
| `21 << 0` | 21 | Move no positions |
| `0 << 31` | 0 | Zero remains representable |
| `1 << 31` | Arithmetic error | 2,147,483,648 exceeds INT32_MAX |
| `1 >> 32` | Arithmetic error | Count equals the width and is outside the contract |
| `-1 >> 1` | Arithmetic error | Negative left inputs are deliberately excluded |

The VM does not expose unsigned types or promise the signed-negative shift behavior of another C compiler. Do not use a machine shift instruction's masked count as an error-handling policy. A request for 32 positions must not silently become a request for zero.

## Check and run an example

Build the existing compiler using the repository's documented CMake preset. From the repository root, with the compiler executable on PATH:

~~~powershell
umicom-c check examples/checked_shifts.uc
umicom-c run examples/checked_shifts.uc --max-steps 12
umicom-c compile examples/checked_shifts.uc checked-shifts.umcb
umicom-c disassemble checked-shifts.umcb
umicom-c exec checked-shifts.umcb --max-steps 12
~~~

The example starts at 84, shifts right by one, shifts left by zero, and returns 42. It uses twelve VM instructions. Both execution commands should print `Result: 42` and `Instructions: 12`. A budget of eleven refuses the final return and must not print a completed result. Compilation refuses to overwrite an existing output; choose a new output filename when repeating the exercise.

A compile or check command only admits source grammar and structure. To see the distinction, write `int main(void){return 1 << 31;}`. Its structure is valid, but running it reports an arithmetic failure at the shift token. No wrapped integer is published.

## Group expressions deliberately

Addition binds more tightly than shifts; comparisons bind less tightly. Thus `1 + 2 << 3` means `(1 + 2) << 3`, producing 24. The expression `1 << 2 + 3` means `1 << (2 + 3)`, producing 32. Parentheses help readers even when precedence already produces the intended grouping.

Repeated shifts associate left to right. `64 >> 2 >> 1` means `(64 >> 2) >> 1`, producing 8. Logical short-circuiting still applies: `0 && (1 << 32)` returns zero without executing the invalid shift. The skipped source must still be syntactically valid and use declared names.

An initialized writable local can use `value <<= count;` or `value >>= count;`. The compiler reads its old value once, evaluates the right expression once, applies the checked operation and stores the result. Read-only locals refuse all such updates during source checking. Updates remain statements; using `return value <<= 1;` is not supported.

## Keep one runtime implementation

The source compiler emits SHIFT_LEFT=22 or SHIFT_RIGHT=23. Each has a zero reserved operand, consumes two stack values and publishes one result. The shared runtime checks the input domain before calculating. It uses widened multiplication or division by a power of two, followed by the existing result-range check. It never executes an unchecked signed host shift.

Ordinary runs, traces, paused debugger steps and instruction slices use this same engine. On an arithmetic refusal, the failed instruction leaves its stack, locals, instruction position and execution count unchanged. Earlier successful instructions remain completed; this is not rollback of the whole program. A caller-owned final result is published only after a successful RETURN. The trace may report the failed attempt.

## Transfer bytecode safely

Any program containing either shift opcode requires canonical wire format 7. The eight-byte header and nine-byte records remain unchanged. Programs without shifts retain their previous minimum format and exact encoding. The encoder scans the entire operation sequence: an earlier bitwise record must not hide a later shift.

The compiler and runtime each advertise `UMC_FEATURE_CHECKED_SHIFTS`, bit 8388608. A runtime receiving serialized code must also advertise bit 7 in `UMC_PROFILE_WIRE_VERSIONS`. Query the component actually linked into the application. New headers do not upgrade an installed library.

Kernel's optional CVM diagnostic uses original literal format-7 records and the shared runtime. Its selectors cover left/right results, negative or excessive counts, overflow, negative values, wrong versions, reserved operands and fuel. Source receipts include the new guest fixture. These tests do not make the VM a general C23 compiler or give guest bytecode access to native addresses.

## Verify and extend

~~~powershell
ctest --test-dir build/windows-ucrt64-debug -R "umicom.c.shifts|umicom.c.cli-shifts" --output-on-failure
~~~

Use the build directory you configured if it differs. The regression suite includes all 32 admitted counts, representable native C comparisons, independent bytes, malformed spans, read-only updates, canonical-version ordering, one-instruction slices and repeated failure without state mutation. Adding negative or unsigned semantics later requires an explicit new contract and tests; it must not silently reinterpret existing programs.
