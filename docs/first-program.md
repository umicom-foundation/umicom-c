# Run your first Umicom C program

A source file contains text. A compiler checks that text and turns it into operations. The Umicom C command can then execute those operations in its small virtual machine.

Use a C23-capable C compiler, CMake 3.24 or newer and Ninja. The Windows preset expects the MSYS2 UCRT64 compiler at `C:\msys64\ucrt64\bin\gcc.exe`. Open PowerShell, then configure and build from your Umicom C checkout:

```powershell
Set-Location "C:\umicom\umicom-c"
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
cmake --preset windows-ucrt64-debug
cmake --build --preset windows-ucrt64-debug --parallel 2
ctest --preset windows-ucrt64-debug --output-on-failure --no-tests=error
```

Run one line at a time and stop on an error. CMake configures the build graph; the build command produces executables; CTest runs the registered checks. An expected result below is something for you to verify, not a report of execution on your machine.

1. Open [arithmetic.uc](../examples/arithmetic.uc). It is a complete program in the supported arithmetic language.
2. Check it without executing:
   `& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" check ".\examples\arithmetic.uc"`.
3. Execute it:
   `& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" run ".\examples\arithmetic.uc"`.
4. The command prints a result and the number of executed instructions. Compare that result with the expression in the file.
5. Try [checked_failure.uc](../examples/checked_failure.uc). A runtime arithmetic error should produce a diagnostic and a nonzero exit status.

## Save and inspect bytecode

Choose a new output filename. Existing files are deliberately preserved.

```powershell
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" compile ".\examples\arithmetic.uc" ".\build\example.umcb"
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" disassemble ".\build\example.umcb"
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" exec ".\build\example.umcb"
```

Disassembly lists the interpreter operations and source byte offsets. Execution should agree with the source run. Use another output name for a second compilation; do not delete your source to clear an output-name error.

## Troubleshooting

An unsupported-source diagnostic can mean valid full C that this small language does not implement. Check [the language reference](language.md), especially the required `main(void)` and single return statement.

If CMake cannot find Ninja or the compiler, check your tool installation and PATH before retrying. A path error is not a source-language error. The current CLI uses narrow standard-C file paths; Unicode path behaviour depends on the host C runtime.

For a portable host build, use the `native-debug` preset after making your compiler and Ninja available. A native host build is not a Kernel cross build and does not prove that a guest boots.
