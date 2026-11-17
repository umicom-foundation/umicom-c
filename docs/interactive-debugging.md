![Umicom](assets/branding/umicom-logo.svg)

# Pause and inspect a small Umicom C program

A debugger lets you stop between operations and inspect computed values. The current console debugger accepts the [Umicom integer language](language.md), not arbitrary C23. It compiles the complete input, validates its bytecode and pauses before the first instruction. It is not a persistent expression REPL: entering a declaration at the prompt does not change the program.

## Start the debugger

From the Umicom C repository, run these PowerShell commands separately. Stop if any command reports an error.

~~~powershell
cmake -S . -B build/debug -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --target umicom-c --parallel 2
& "./build/debug/bin/umicom-c.exe" debug examples/local_integers.uc
~~~

At the prompt, type **where** and press Enter. This displays the next instruction and its source byte location. Type **step** to execute one instruction. Type **locals** to display initialized local slots. A slot is a numbered storage location; variable names are not retained in this display.

| Command | Effect |
|---|---|
| step or an empty line | Execute one bytecode instruction |
| line | Continue until the next instruction has a different source line |
| where | Show the next instruction without execution |
| locals | Show initialized integer slots |
| continue | Run within the remaining instruction allowance |
| quit | Cancel without publishing a completed result |
| help | Show available commands |

An expression can produce several instructions. Several statements on one physical source line are grouped by **line**; a multiline expression can stop partway through. This groups source coordinates, not complete C statements. Use **step** for exact operation boundaries.

## Observe success and failure

Create a file named lesson.uc in a scratch directory:

~~~c
int main(void) {
    int answer = 40;
    answer += 2;
    return answer;
}
~~~

Debug that file. Use **line**, then **locals**, and repeat. The local slot changes from 40 to 42. Use **continue**; successful completion prints **Result: 42**.

Change the return expression to **answer / 0**. Compilation can accept the expression, but execution refuses division by zero. The failed instruction does not modify the machine. No completed result is printed. This demonstrates the difference between a source error and an execution error.

Add **--max-steps 1** after the file path to practice fuel exhaustion. Fuel counts VM instructions, including RETURN. It is not milliseconds. A depleted budget does not prove the algorithm is wrong. Reopen the source with an appropriate budget.

For an existing artifact, use **debug-bytecode program.umcb**. Bytecode contains source offsets but not source text; its **line** command advances one instruction. Keep the exact original source for source-line interpretation.

## Embed the stepper

Include **umicom_c/machine.h** and link **UmicomC::Runtime**; also link **UmicomC::Compiler** when compiling source. Allocate an **UmcMachine** outside a small embedded stack. Call **umc_machine_init** with valid storage, the program and fuel. Initialization copies the program, so its input may be released after success.

Call **umc_machine_step** for each operation. Inspect status before reading a trace. Call **umc_machine_result** only after RETURN. Failure preserves the result output; an old value must not be presented as a new success. Do not mutate machine fields, share a machine concurrently, serialize its host layout or alias input/output buffers.

Framework supplies **umi_cvm_debug_start**, **umi_cvm_debug_step** and **umi_cvm_debug_cancel** in its optional CVM adapter. It invalidates stale presentation results and retains source coordinates across pauses. Studio UI wiring and graphical breakpoints are separate application work.

## Limits

This language has bounded integers, locals and forward conditional control flow. It has no arbitrary pointers, file I/O, threading, native calls or full C23 implementation. A VM pass cannot qualify a GTK application, detect every native memory leak or replace native tests. Follow [the execution course](execution-course.html) to choose the right feedback path.
