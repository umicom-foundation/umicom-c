![Umicom](assets/branding/umicom-logo.svg)

# Write and check numeric literals

A literal is a value written directly in source. Umicom C accepts decimal digits,
hexadecimal after 0x or 0X, and binary after 0b or 0B. The spelling changes how a
person reads the value; the runtime still receives a signed 32-bit integer.

## Build the compiler and its checks

Start in the Umicom C source directory with CMake, Ninja and a C23 compiler on PATH.
Use a build folder dedicated to this checkout. Stop after any command fails.

~~~powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DUMICOM_C_BUILD_TESTS=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure --no-tests=error
.\build\bin\umicom-c.exe profile
~~~

If CMake reports a generator or source-directory mismatch, use a new build folder.
<!-- Earlier profile wording retained for review:
Do not point a cache at a second source checkout. The profile reports compiler.features=103:
1 for arithmetic, 2 for comparison, 4 for logical negation, 32 for hexadecimal and
64 for binary literals. The runtime mask remains 27. These are declarations of the
linked implementation, not an authenticated identity or a complete C23 claim.
-->
<!-- Earlier profile statement retained for review:
Do not point a cache at a second source checkout. Check the hexadecimal and binary feature bits (32 and 64) rather than assuming the entire compiler mask is fixed. The current local, logical, conditional-expression, compound-assignment and conditional-statement implementation reports compiler.features=16103 and runtime.features=411; see [capability profiles](capability-profiles.md). Literal spelling itself adds no runtime opcode. These declarations identify supported behavior, not authenticated binary identity or complete C23 compatibility.
-->
<!-- Earlier capability snapshot retained for review: Do not point a cache at a second source checkout. Check the hexadecimal and binary feature bits (32 and 64) rather than assuming the entire compiler mask is fixed. The current local, logical, conditional-expression, compound-assignment and conditional-statement implementation reports compiler.features=65255 and runtime.features=16795; see [capability profiles](capability-profiles.md). Literal spelling itself adds no runtime opcode. These declarations identify supported behavior, not authenticated binary identity or complete C23 compatibility. -->
<!-- Earlier pre-increment/separator compiler snapshot retained for review: Do not point a cache at a second source checkout. Check the hexadecimal and binary feature bits (32 and 64) rather than assuming the entire compiler mask is fixed. The current local, logical, conditional-expression, compound-assignment and conditional-statement implementation reports compiler.features=130791 and runtime.features=82331; see [capability profiles](capability-profiles.md). Literal spelling itself adds no runtime opcode. These declarations identify supported behavior, not authenticated binary identity or complete C23 compatibility. -->
Do not point a cache at a second source checkout. Query the hexadecimal and binary feature bits (32 and 64), and the digit-separator bit (262144), from the linked compiler. <!-- Earlier source-feature snapshot retained for review: The current source implementation reports compiler.features=524007 and runtime.features=82331; see [capability profiles](capability-profiles.md). -->
Inspect the linked implementation’s required source bits and independent runtime features; see [capability profiles](capability-profiles.md). Source spelling changes add no runtime opcode. These declarations describe supported behavior, not authenticated binary identity or complete C23 compatibility.

These complete masks include the separate [checked remainder](remainder.md) extension. Remainder adds opcode 18 and format 5; the example in this guide retains its existing execution and minimal wire format.

## Compare two spellings

The checked example is [integer_literals.uc](../examples/integer_literals.uc):

~~~c
int main(void) {
    return 0x2A == 0b101010;
}
~~~

Hexadecimal 2A is two sixteens plus ten, or 42. Binary 101010 is 32 + 8 + 2,
also 42. Equality therefore returns 1. From the same source directory:

~~~powershell
.\build\bin\umicom-c.exe check examples\integer_literals.uc
.\build\bin\umicom-c.exe run examples\integer_literals.uc --max-steps 4
.\build\bin\umicom-c.exe compile examples\integer_literals.uc build\integer_literals.umcb
.\build\bin\umicom-c.exe exec build\integer_literals.umcb --max-steps 4
.\build\bin\umicom-c.exe trace-bytecode build\integer_literals.umcb
~~~

Expect four instructions: push 42, push 42, compare, return. Both execution commands
print Result: 1 and Instructions: 4. The artifact uses the existing comparison format 2.
A constant by itself still uses arithmetic format 1. No new instruction is needed
for either base. The CLI refuses an existing output path; choose a fresh output
name when repeating the compile command.

## Predict a refusal before running it

Make a separate practice source and change only its return expression.

| Expression | Expected behavior |
| --- | --- |
| 0x7fffffff | largest admitted positive literal, 2147483647 |
| -0x7fffffff - 1 | smallest signed result, -2147483648 |
| 0x80000000 | literal range error before execution |
| -0x80000000 | same range error; unary minus does not widen the token |
| 0b102 | syntax error at the digit 2 |
| 0x | syntax error at the incomplete token start |
| 0x1u | suffix is unsupported |
| 010 | decimal ten, preserving the earlier language profile |

A radix is the number of digit values in a spelling: 2, 10 or 16 here. Hexadecimal
accepts A through F in either case. Signs remain separate unary operators.
<!-- Earlier restriction retained for grammar history: Unsigned types, octal, suffixes, digit separators and floating-point forms are
outside this bounded language. -->
Unsigned types, octal, suffixes and floating-point forms are outside this bounded language. A source file using those features needs a different
compiler or an explicit rewrite; do not silently reinterpret it.

## Check before multiplying

The lexer extends value by multiplying by the radix and adding the next digit.
It first checks value > (INT32_MAX - digit) / radix. If true, the new value cannot
fit, so it reports a range error without performing the overflowing operation.
This order matters: checking after signed overflow is too late.

Read [lexer.c](../src/lexer.c), then locate prefixed_integer. Each lookahead checks
the supplied length. The routine does not call a string conversion function,
assume a trailing NUL, allocate memory, or use a locale to classify digits.
Unprefixed numbers continue through the earlier decimal path.

## Separate source and artifact evidence

Run the focused checks after building:

~~~powershell
ctest --test-dir build -R "umicom.c.(integer-literals|cli-integer-literals|profile|cli-profile)" --output-on-failure --no-tests=error
~~~

The five library groups check values, refusals and locations, source spans, exact
wire bytes and decimal compatibility. The CLI case checks compilation, source/wire
execution, instruction budgets and refusal to publish an artifact after invalid
source. Profile cases exercise the features they declare.

A source span is a pointer plus a byte count. The span test copies bytes into an
array that has no NUL terminator. That array is valid input to umc_compile with its
size, but is not valid input to strlen. An earlier direct string initializer for
that fixture is retained for review; the active explicit copy works with strict
compiler string-initialization warnings without disabling them.

Passing these checks establishes the exercised profile on the selected build.
It does not qualify a Kernel target, an editor integration, all C syntax or an
arbitrary older runtime binary.

<!-- Earlier complete compiler mask statements retained for review before source complement:
Do not point a cache at a second source checkout. Check the hexadecimal and binary feature bits (32 and 64) rather than assuming the entire compiler mask is fixed. The current local, logical, conditional-expression, compound-assignment and conditional-statement implementation reports compiler.features=32487 and runtime.features=16795; see [capability profiles](capability-profiles.md). Literal spelling itself adds no runtime opcode. These declarations identify supported behavior, not authenticated binary identity or complete C23 compatibility.
-->

## Group long numbers without changing their value

A digit separator is a single apostrophe between two digits in the same number.
It helps a person read a long spelling: `1'000` is one thousand and
`0b1010'1010` is the same value as `0b10101010`. It is not a string, character
literal, subtraction or a new runtime operation.

1. Open [digit_separators.uc](../examples/digit_separators.uc). It compares
   hexadecimal `0x2'A` with binary `0b10'1010`.
2. Read each side as 42. Predict that equality will produce 1.
3. Run the commands below from the compiler source directory after building.
4. Inspect the trace. The apostrophes add no instructions: the program still
   pushes two numbers, compares them and returns.

~~~powershell
.\build\bin\umicom-c.exe check examples\digit_separators.uc
.\build\bin\umicom-c.exe run examples\digit_separators.uc --max-steps 4
.\build\bin\umicom-c.exe compile examples\digit_separators.uc build\separated-example.umcb
.\build\bin\umicom-c.exe exec build\separated-example.umcb --max-steps 4
ctest --test-dir build -R "umicom.c.(digit-separators|cli-digit-separators)" --output-on-failure --no-tests=error
~~~

Choose another output filename if the artifact already exists; the compiler
refuses to overwrite it. Expect Result: 1 and Instructions: 4 from run and exec.

| Try this spelling | Reason and expected result |
| --- | --- |
| `2'147'483'647` | largest admitted literal, still signed 32-bit |
| `2'147'483'648` | range error; separators do not enlarge the integer |
| `0'010` | decimal ten; leading zeros do not enable octal |
| `0x'2A` | syntax error: a separator cannot follow the prefix |
| `42'` | syntax error: a separator needs a following digit |
| `4''2` | syntax error: two separators are not two digits |
| `0b1'2` | syntax error: 2 is not a binary digit |
| `4' 2` | syntax error: whitespace cannot be inside the number |

The error for a misplaced apostrophe identifies that apostrophe's byte offset.
A range error identifies the beginning of the number. The lexer checks its input
length before looking at the next byte. It validates that byte in the current
radix before skipping the separator, then uses the existing checked multiply/add.
No allocation, locale conversion or VM change is involved.

The linked compiler advertises `UMC_FEATURE_DIGIT_SEPARATORS` (262144).
The runtime does not advertise that source-only bit. An old compiler may reject
this spelling even when its runtime can execute the resulting ordinary integer
instructions; check the linked implementation, not just a new header.
