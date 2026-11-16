![Umicom](assets/branding/umicom-logo.svg)

# Change an integer by one

An integer is a whole number. A variable gives that number a name. An initialized variable has already received a value. This lesson uses the experimental Umicom C compiler, which supports a deliberately small part of C rather than arbitrary C23 programs.

## 1. Create a program

Save the following as counter.uc in your working directory. A semicolon finishes a statement. Braces enclose the body of main, the function this compiler runs.

~~~c
int main(void)
{
    int value = 41;
    ++value;
    return value;
}
~~~

The declaration creates value and stores 41. The two adjacent plus signs mean add one. Return gives the completed result to the caller. The expected result is 42. Writing value++ as its own statement gives the same result here. Similarly, --value and value-- subtract one. Spaces or comments may surround the pair, but must not split it.

<!-- The initial executable example omitted the configured bin directory: C:\umicom\umicom-c\build\windows-ucrt64-debug\umicom-c.exe. Retained here for diagnosis; the active command follows CMake RUNTIME_OUTPUT_DIRECTORY. -->

## 2. Check, run and inspect

In PowerShell, set the executable path to the compiler you built. The ampersand invokes that path. These commands read your source; compile creates a new artifact, so choose an unused output name.

~~~powershell
$UmicomC = "C:\umicom\umicom-c\build\windows-ucrt64-debug\bin\umicom-c.exe"
& $UmicomC check .\counter.uc
& $UmicomC run .\counter.uc --max-steps 8
& $UmicomC trace .\counter.uc
& $UmicomC compile .\counter.uc .\counter.umcb
& $UmicomC exec .\counter.umcb --max-steps 8
~~~

If your executable is elsewhere, use its actual path. An unrecognized-command error means PowerShell could not start it; it is not a compiler diagnosis of counter.uc. Use Test-Path on the executable first. Read execution-guide.md when you have not built the compiler yet.

This small program has eight instructions. Initialization needs two, the update needs four, and return needs two. Running with seven steps must fail with instruction-budget exhaustion instead of publishing 42. The budget, also called fuel, limits VM operations; it does not count source lines or measure elapsed time.

| Update operation | Meaning |
|---|---|
| LOAD_LOCAL | Read the existing number |
| PUSH 1 | Put the constant one on the evaluation stack |
| ADD or SUBTRACT | Perform checked signed arithmetic |
| STORE_LOCAL | Replace the local only after arithmetic succeeds |

These are existing runtime operations. A program using only these local operations keeps canonical bytecode format 3. If a surrounding conditional needs branches, its existing format 4 applies. No new instruction needs to be implemented in an embedded runtime simply to understand this spelling.

## 3. Explore a controlled failure

Replace 41 with 2147483647, the largest signed 32-bit integer. Increment would exceed that range, so execution reports an arithmetic error at the update operator. It does not wrap to a negative value. Decrementing -2147483648 is also refused. Callers of the C API retain their previous result object when execution fails. Trace records show only successful stores before the failing arithmetic operation.

## 4. Know the syntax boundary

Use updates only as complete statements on initialized locals. Do not write return value++, int next = ++value, ++(value), or ++unknown. Such expressions are not implemented. Both branches of an if statement must compile, even if the condition makes one branch unreachable at runtime. This is not the full prefix/postfix expression behavior of ISO C.

## 5. Verify your build

~~~powershell
ctest --test-dir .\build\windows-ucrt64-debug -R "umicom.c.(increment|cli-increment)" --output-on-failure
~~~

The regression groups cover normal values, syntax refusals, overflow, exact fuel, bytecode round trips, bounded source spans, instruction capacity and comparison with native C updates. A test listing is not a pass; read the execution result from your selected build. The complete six-update example is examples/increment.uc and needs 28 instructions, which differs from the eight-instruction teaching program above.

## Compatibility when signs touch

The lexer now reads adjacent ++ or -- as one update token. Older experimental sources such as return +--7 must separate the unary signs: return +- -7. This preserves the intended unary arithmetic. Adjacent updates in return expressions are rejected because this implementation supports only standalone update statements.
