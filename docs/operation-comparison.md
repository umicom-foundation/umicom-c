![Umicom](assets/branding/umicom-logo.svg)

# Compare source operations without executing them

Use compare-operations to ask whether two accepted Umicom C programs compile to
the same opcode and operand sequence. The command calls the existing
umc_program_same_operations library service; it does not execute either input.
This is instruction identity, not a general proof that two C programs are equivalent.

## Before you begin

Build the hosted command using the [execution guide](execution-guide.md).
Use two ordinary source files that fit the current bounded language. The command
accepts source, not a bytecode artifact. It requires exactly two paths and has no
execution-budget option. Quote paths containing spaces.

1. Save this as left.uc:

~~~c
int main(void) { return 0x2a; }
~~~

2. Save this as right.uc:

~~~c
/* A different spelling and diagnostic location. */
int main(void) { return 0b101010; }
~~~

3. Run your built executable, adjusting its path if necessary:

~~~powershell
.\build\windows-ucrt64-debug\bin\umicom-c.exe compare-operations .\left.uc .\right.uc
$LASTEXITCODE
~~~

Expected output includes operations=same, two instructions for each program,
source-locations=not-compared and execution=not-performed. The exit code is zero.
The command reads both inputs and creates no output files.

## Interpret all three exits

| Exit | Meaning |
| --- | --- |
| 0 | Both inputs compiled and have identical validated opcode/operand sequences. |
| 1 | Both inputs compiled, but their opcode/operand sequences differ. |
| 2 | Usage, input reading, compilation, comparison validation or output reporting failed. |

These are this command's exit meanings. Other existing commands retain their
own behavior; for example, run reports arithmetic failure as a command failure.
Do not treat every nonzero compare-operations result as malformed source.

Replace right.uc with return 43 in the same function body. Expected result:
operations=different, exit one. Next remove the return expression. Expected
result: a diagnostic naming right.uc, exit two and no operation comparison report.
An invalid left input is reported with the left path instead.

## Understand the limits of sameness

return 2 + 3 and return 5 produce the same final value when executed, but their
instruction sequences differ. This compiler does not erase the addition.
Conversely, two copies of return 1 / 0 have identical operations and are accepted
by this command; execution would fail at division. No runtime pass is claimed.

Comments and whitespace can move diagnostic source offsets without changing
operations. Encoded bytecode retains offsets, so identical operations need not
mean identical serialized bytes. The library's comparison deliberately ignores
those offsets after validating both programs.

Do not use this command alone to reuse an executable, skip tests or publish an
artifact. Compiler inputs, flags, generated files, dependencies, diagnostics and
artifact identity remain separate concerns. See [configuration input evidence](configuration-inputs.md).

## Regression and troubleshooting

~~~powershell
ctest --test-dir build/windows-ucrt64-debug -R '^umicom[.]c[.]cli-operation-identity[.]' --output-on-failure --no-tests=error
~~~

Use your configured build directory. Expected result: twelve groups pass,
including changed comments, different numeric spelling, equal results with
different operations, an unexecuted arithmetic failure, missing/invalid input,
oversized source and incorrect arity. Each group checks input hashes and confirms
that the command creates no output file.

If a comparison differs unexpectedly, inspect both programs with inspect and
compile/disassemble copies into new output paths. Keep the originals intact.
If source reading fails, check the exact path and source-size limit before changing
language or comparison rules. A diagnostic is evidence to investigate, not permission
to treat an incomplete compilation as a valid program.

## A comparison report must be written successfully

The command flushes its short stdout report before returning 0 or 1. A flush or
stream error returns 2, even when the instructions were successfully compared.
This observes the C stream write result; it does not promise storage durability.

`tests/test_cli_report_failure.c` invokes the actual CLI C entry with either its
normal stdout or a native read-only descriptor. Four cases cover both semantic
outcomes with and without the output fault, and compare input bytes afterward.
The Windows and POSIX descriptor adapters are test-only. A run on one platform
does not qualify the other adapter or an installed shell pipeline.
