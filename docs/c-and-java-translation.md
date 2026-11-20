![Umicom](assets/branding/umicom-logo.svg)

# Translate an integer function between C and Java

The translator supports one function with no parameters and a final
integer return. The C spelling is `int answer(void)`. The Java
spelling is `public static int answer()` inside `public final class
UmicomAnswer`. The generated C function requires a host with a 32-bit
`int`.

## Run a round trip

Build Umicom C using the Windows debug preset. In a fresh work folder,
create `answer.c` containing this definition.

~~~c
int answer(void) {
    return (10 + 4) * 3;
}
~~~

Run the following commands with the built Umicom C executable on your path.
Choose unused output filenames. Each command exits with zero on success.

~~~powershell
umicom-c translate c-to-java answer.c UmicomAnswer.java
umicom-c translate java-to-c UmicomAnswer.java answer-roundtrip.c
~~~

Open the two generated files. Both functions return 42. Compile
`UmicomAnswer.java` with a JDK and call `UmicomAnswer.answer()` from a
Java test program. Compile the C file with a native C compiler and call
`answer()` from a C test program. These functions return values to their
callers. A program's process exit code is a separate convention.

## Use initialized local integers

In a fresh exercise folder, replace the body with these steps. Keep the
same two translation commands and use new output names if the previous
files are still present.

~~~c
int answer(void) {
    int net = 6;
    int fee = 3;
    net = (net + fee) * 5;
    return net - fee;
}
~~~

The expected result is still 42. Declare each local with an initial value
before reading it. Use an ASCII name starting with a letter and simple
assignments with `=`. The translator checks each calculation before
writing a complete result. The function can contain up to 32 statements
including the final return, subject to the compiler's source and
instruction limits.

## Preserve comments inside the function

Use a fresh exercise folder and put this body in `answer.c`.

~~~c
int answer(void) {
    /* Keep the reason for the adjustment beside the calculation. */
    int subtotal = 20;
    subtotal = subtotal + 1; // Include the adjustment.
    return subtotal * 2;
}
~~~

Run the two translation commands from the round trip. The C and Java
outputs both return 42 and keep the body text, including these two
comments and its line endings. The shared compiler checks the arithmetic
with a private view of the source. Errors still identify the original
line and column.

A commented function uses the 4,096-byte function limit. Put comments
inside the braces and use ASCII text. Java Unicode escapes and C line
splices can alter the surrounding program before either compiler reads
a comment, so the translator rejects them. Use a plain note and retry
with an unused output path.

## Check a refusal

Enter `2147483647 + 1` as the return expression and try a new output path.
The translator reports an arithmetic error at the original source location
and creates no file. Java and C can handle overflowing expressions
differently. Umicom C checks the shared integer semantics before emitting
either target.

The accepted expression uses ASCII decimal literals from 0 through
2147483647, parentheses and the checked arithmetic operators. Leading
zeroes, suffixes and Java objects are outside this profile. The original
expression-only API also requires an expression free of comments.
The local profile also refuses reserved names, uninitialized locals,
increment operators and control flow. Reusing an output path is refused
to preserve earlier work.

The hosted tool calls [the same compiler API](../include/umicom_c/translate.h)
that another Umicom client can use. Run the native tests from the
configured source checkout.

~~~powershell
ctest --test-dir build/windows-ucrt64-debug --output-on-failure --no-tests=error -R '^umicom[.]c[.]translation[.]'
~~~

These cases check both directions, bounded output, diagnostic locations and
refusals. General Java and C projects need additional translation rules and
target compiler checks.
