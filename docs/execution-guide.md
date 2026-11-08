# Build and use Umicom C

Umicom C currently implements a small arithmetic language and a bounded virtual
machine. It does not yet compile the Kernel, Framework or Applications. Those
projects use their existing C23 toolchains. This guide explains the prototype
without requiring any compiler-construction background.

## 1. From text to a result

```text
source.uc -> lexer -> compiler -> instructions -> virtual machine -> result
                                  |
                                  +-> program.umcb -> decoder -> virtual machine
```

A **lexer** finds tokens, such as the number `6` and the multiplication sign.
The compiler checks their arrangement and emits instructions. **Bytecode** is
a stored representation of those instructions. A virtual machine interprets
them; it is not a simulated whole computer and the file is not a boot image.

## 2. Build on Windows

Install the MSYS2 UCRT64 GCC, CMake and Ninja tools. Open PowerShell and use
the checkout below, or replace it with your chosen location. Run one command
at a time and stop on a nonzero `$LASTEXITCODE`.

```powershell
Set-Location "C:\umicom\umicom-c"
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
$LogRoot = "C:\umicom\_local\logs\umicom-c-$(Get-Date -Format yyyyMMdd-HHmmss)"
New-Item -ItemType Directory -Force $LogRoot
cmake --preset windows-ucrt64-debug -DBUILD_TESTING=ON 2>&1 | Tee-Object "$LogRoot\configure.log"
$LASTEXITCODE
cmake --build --preset windows-ucrt64-debug --parallel 2 2>&1 | Tee-Object "$LogRoot\build.log"
$LASTEXITCODE
ctest --preset windows-ucrt64-debug --output-on-failure --no-tests=error --output-log "$LogRoot\tests.log"
$LASTEXITCODE
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" --version
```

Configuration chooses the compiler. Building translates source files. Testing
checks behaviour. Keep the existing build directory for incremental builds:
Ninja tracks changed inputs and dependencies. A compiler/toolchain change needs
a separate build directory, not edits to the cached compiler in an old one.

On a Linux development host with a suitable C23 compiler, CMake and Ninja, use
`native-debug` for all three preset commands. The executable is
`./build/native-debug/bin/umicom-c`. This is a host workflow, not a command that
can currently run inside the small Umicom OS recovery guest.

## 3. Write one supported program

In your editor create a new folder you choose for examples. Save this text as
`answer.uc`, with UTF-8 encoding without a byte-order mark:

```c
/*-----------------------------------------------------------------------------
 * Umicom C Learning Example
 * File: answer.uc
 *
 * PURPOSE:
 *   Return a small arithmetic result for the instruction-budget exercise.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
int main(void) { return 6 * 7; }
```

`int` describes an integer result. `main` names the supported entry function.
`void` states that it takes no arguments. Braces contain the body. `return`
supplies the answer; `*` multiplies and the semicolon ends the statement.
The comment is documentation, not an executed instruction.

For example, if you saved it at `C:\umicom\examples\umicom-c\answer.uc`:

```powershell
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" check "C:\umicom\examples\umicom-c\answer.uc"
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" run "C:\umicom\examples\umicom-c\answer.uc" --max-steps 4
```

Expected successful execution prints `Result: 42` and `Instructions: 4`.
The four instructions push 6, push 7, multiply, then return. A **stack** holds
temporary values: after the first two pushes it contains 6 and 7; multiplication
replaces those operands with 42.

## 4. Store and inspect the program

```powershell
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" compile "C:\umicom\examples\umicom-c\answer.uc" "C:\umicom\examples\umicom-c\answer.umcb"
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" disassemble "C:\umicom\examples\umicom-c\answer.umcb"
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" exec "C:\umicom\examples\umicom-c\answer.umcb" --max-steps 4
```

Compilation refuses an existing output file. Choose a new output name when
recompiling; do not overwrite your source. **Disassembly** lists bytecode
instructions in readable form. `exec` should give the same result as `run`.

## 5. Observe a controlled failure

```powershell
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" run "C:\umicom\examples\umicom-c\answer.uc" --max-steps 3
$LASTEXITCODE
```

Three steps are insufficient to execute the return instruction. Expect a
diagnostic, exit code 1 and no published `Result:` line. This intentional
exercise shows that completing the multiplication is different from completing
the program. An instruction budget is not a wall-clock timeout or an operating
system security sandbox.

<!-- Earlier command scope, retained for review after adding trace commands:
`&#45;&#45;max-steps` accepts a decimal integer from 1 to 256 on `run` and `exec` only.
-->
`--max-steps` accepts a decimal integer from 1 to 256 on `run`, `exec`, `trace` and `trace-bytecode`.
The default remains 256. Zero, negative values, excessive values, repeated
options and options on `check` are rejected. Exit code 2 reports command/input
usage failures; code 1 reports compilation or runtime diagnostics.

## 6. Read and extend the implementation

| File | What to read first |
| --- | --- |
| `tools/main.c` | Command selection, file ownership and budget parsing |
| `src/lexer.c` | How source characters become tokens |
| `src/compiler.c` | How arithmetic becomes instructions |
| `src/vm.c` | Validation, stack operations and instruction accounting |
| `include/umicom_c/umicom_c.h` | Limits, types and public function contracts |
| `tests/check_cli_budget.cmake` | Real CLI budget and input-preservation checks |

**Exercise:** predict the result of `return (6 + 1) * 2;` before running it.
**Answer:** 14. Parentheses request addition first. Use disassembly to count
instructions rather than assuming every expression uses four steps.

Read [the first program guide](first-program.md) next for the existing language
and bytecode boundaries. Adding variables, types, native code generation or
full C23 compatibility requires new language rules and tests; the arithmetic
profile does not already provide those capabilities.

## 7. Observe execution without changing the format

Continue with [execution traces](execution-traces.md). `trace` compiles and executes source; `trace-bytecode` decodes and executes an existing artifact. Both share the same checked runtime as `run` and `exec`. A failed arithmetic or fuel step remains visible, while the final result is published only on success. This makes a predicted stack table directly comparable to runtime observations.

## 8. Compare two arithmetic results

The comparison example is part of the repository. After rebuilding the compiler, run:

```powershell
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" inspect ".\examples\comparison.uc"
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" trace ".\examples\comparison.uc" --max-steps 8
ctest --preset windows-ucrt64-debug -R "comparison" --output-on-failure --no-tests=error
```

The expression `2 + 3 < 2 * 3` completes both arithmetic operands before comparing 5 with 6. Expect eight instructions, a peak stack of three, four constants, two arithmetic operations and eighty encoded bytes. The final result should be 1. A false comparison returns 0 with a successful command exit; the numeric answer and process status have different meanings.

The operators `<`, `<=`, `>`, `>=`, `==` and `!=` are supported. Both operands execute. There are still no variables, conditionals, calls or logical short-circuit operators. Changing the source to `0 == (1 / 0)` should fail at the division without an equality or return result.

Comparison artifacts use wire format 2. Existing arithmetic programs keep their exact format-1 encoding. Use a fresh output name if you compile this example, and qualify the runtime used by every dependent consumer. An older runtime rejecting format 2 is a compatibility refusal, not a reason to alter the artifact header manually.
