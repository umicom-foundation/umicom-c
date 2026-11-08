# Inspect a program before execution

This module follows one small program from text to an inspectable instruction sequence. Each lesson defines its terms, shows a concrete operation and gives an exercise with an expected observation. Inspection describes the program; execution attempts its arithmetic. A compiler, an IDE and a teaching tool can all use the same library contracts.

## Set up the compiler

A terminal is a window in which commands are typed. A compiler translates source text into instructions; CMake describes a build and Ninja carries out its steps. Install Git and the MSYS2 UCRT64 GCC, CMake and Ninja packages if these tools are not available. The Learning Build Systems course in Bits2Banking explains their roles in more depth.

1. Keep the Umicom C and Bits2Banking checkouts beside each other in a folder you choose. The examples use C:\umicom. Replace that location consistently if yours differs.
2. In the MSYS2 UCRT64 terminal, install the tools with the following command. Complete the package manager's normal update instructions first if it requests them.

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

3. Open PowerShell and build the small compiler. Run each line separately. If a command reports an error, stop and read it before continuing.

```powershell
Set-Location "C:\umicom\umicom-c"
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
cmake --preset windows-ucrt64-debug -DBUILD_TESTING=ON
cmake --build --preset windows-ucrt64-debug --parallel 2
ctest --preset windows-ucrt64-debug --output-on-failure --no-tests=error
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" inspect ".\examples\inspection.uc"
```

On Linux, install a C23-capable C compiler, CMake and Ninja with your distribution's package manager. From the umicom-c checkout use the native-debug preset:

```bash
cmake --preset native-debug -DBUILD_TESTING=ON
cmake --build --preset native-debug --parallel 2
ctest --preset native-debug --output-on-failure --no-tests=error
./build/native-debug/bin/umicom-c inspect ./examples/inspection.uc
```

The inspection example should report 7 instructions, a peak stack of 3 values, 3 constants, 3 arithmetic operations and 71 encoded bytes. These are expected results to check on your machine. The language currently supports one integer-returning main function with arithmetic; it does not compile the full C language. If inspect is an unknown command, check that the binary was rebuilt from source containing umc_inspect. Keep any previous logs when investigating a failure.


## Inspect source and bytecode

```powershell
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" inspect ".\examples\inspection.uc"
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" compile ".\examples\inspection.uc" "inspection-study.umcb"
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" inspect-bytecode "inspection-study.umcb"
```

Use a new output filename for compilation. Existing files are preserved. Source inspection compiles the supported source language in memory; bytecode inspection decodes the stored format. Neither command executes the program's arithmetic.

| Report field | Meaning |
| --- | --- |
| Instructions | Operation count, including RETURN |
| Peak stack values | Largest number of temporary integers needed |
| Constants | Number of PUSH operations |
<!-- Earlier arithmetic-only definition: | Arithmetic operations | Binary operations plus NEGATE | -->
| Arithmetic operations | ADD, SUBTRACT, MULTIPLY, DIVIDE and NEGATE; excludes comparisons |
| Encoded bytes | Eight-byte header plus nine bytes per instruction |

## Use the library

```c
UmcProgramInfo info;
UmcDiagnostic diagnostic;
UmcStatus status = umc_inspect(program, count, &info, &diagnostic);
if (status == UMC_OK) {
    printf("Peak stack: %zu\n", info.peak_stack);
}
```

Here program must point to count valid instruction objects, with count within the public limit. The caller keeps the input immutable during the call. The report and optional diagnostic must not overlap the program or each other. The output report remains unchanged when inspection fails. A diagnostic message uses static storage and must not be freed.

The same validation walk serves umc_validate, umc_inspect and the validation stage of umc_execute. The old standalone validator is preserved in a disabled block for review. <!-- Earlier inspection-only scope: No bytecode format or opcode changed. -->
Inspection itself does not execute instructions. The later comparison extension appends opcodes and selects wire format 2 as described in the bytecode guide.

## Understand the limits

A valid structure can still divide by zero or exceed the signed 32-bit arithmetic range. Inspecting return 1 / 0; succeeds; running it fails. This is deliberate separation of structure and execution. A report is not a security certification or a native machine-code performance estimate.

The current profile has no branches or loops. A successful run therefore needs instruction_count fuel. A smaller allowance produces fuel exhaustion, and malformed input is rejected even when the budget is zero. If the instruction set gains control flow, budget analysis must be redesigned for that profile.

## Check the behavior

```powershell
ctest --preset windows-ucrt64-debug -R "^umicom[.]c[.](inspection|cli-inspection)$" --output-on-failure --no-tests=error
```

The regression groups cover stack and instruction limits, unknown opcodes, reserved operands, early return, unchanged output on failure, wire-size agreement and command argument rejection. A passing test report must come from executing these commands against the relevant source and toolchain. The commands above are instructions to run, not an execution record.

## Count comparisons separately from arithmetic

For `int main(void) { return 2 + 3 < 2 * 3; }`, the expected report is eight instructions, a peak stack of three values, four constants, two arithmetic operations and eighty encoded bytes. The LESS instruction is included in the total instruction count and consumes two stack values before pushing one. It is excluded from `arithmetic_count`, which retains its earlier meaning. The final RETURN is also included in the instruction count.

Inspection cannot tell you that the comparison will finish successfully: an operand can still divide by zero or overflow. For example, `0 == (1 / 0)` has a valid structure but execution fails before equality. The same source, instruction and stack limits apply to both profiles. Qualify the comparison cases with `ctest --preset windows-ucrt64-debug -R "comparison" --output-on-failure --no-tests=error`; source presence is not an executed result.
