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
Do not point a cache at a second source checkout. Check the hexadecimal and binary feature bits (32 and 64) rather than assuming the entire compiler mask is fixed. The current local, logical and conditional implementation reports compiler.features=1767 and runtime.features=411; see [capability profiles](capability-profiles.md). Literal spelling itself adds no runtime opcode. These declarations identify supported behavior, not authenticated binary identity or complete C23 compatibility.

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
Unsigned types, octal, suffixes, digit separators and floating-point forms are
outside this bounded language. A source file using those features needs a different
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
