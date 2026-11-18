![Umicom](assets/branding/umicom-logo.svg)

# Plan a bytecode instruction budget

An instruction is one operation carried out by the virtual machine. A budget, also called fuel, is the maximum number of instructions a particular run may execute. It prevents a program from consuming more interpreter steps than its caller permits. It does not measure seconds, bytes, electricity or money.

Umicom C can calculate conservative path lengths without executing the program. The shared runtime service validates the complete instruction graph and counts the shortest and longest structural routes from entry through the final RETURN. Every instruction on a route counts, including RETURN. Conditional branches contribute both alternatives even when their current values would select only one.

## 1. Build the native tool

Use the normal [execution guide](execution-guide.md) to configure and build the Windows UCRT64 or native Linux preset. Stop if a command fails. The following Windows commands run from the Umicom C checkout, whose built tool is under build/windows-ucrt64-debug/bin. Substitute the native-debug path on Linux.

## 2. Read the small source example

Open [examples/path_budget.uc](../examples/path_budget.uc). Its program is:

```c
int main(void) { return 1 ? 40 + 2 : 7; }
```

The conditional expression has three parts: a condition before ?, a true value before :, and a false value after :. A nonzero condition chooses the true expression. This program returns 42. The analyzer deliberately does not evaluate that condition or the addition.

## 3. Request the structural report

```powershell
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" budget examples/path_budget.uc
```

Expected output:

```text
Structural minimum instructions: 4
Structural maximum instructions: 7
Both branch alternatives are considered; arithmetic has not been executed.
```

The compiled graph has eight records, but no execution traverses all eight:

```text
0 PUSH 1
1 JUMP_IF_ZERO 6
   true route:  2 PUSH 40 -> 3 PUSH 2 -> 4 ADD -> 5 JUMP 7
   false route: 6 PUSH 7
7 RETURN
```

The true route visits 0,1,2,3,4,5,7: seven instructions. The false route visits 0,1,6,7: four. The latter is structurally present even though this particular source condition is constant and true. The report is therefore a bound, not a prediction of the chosen route.

## 4. Compare planning with execution

```powershell
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" run examples/path_budget.uc --max-steps 7
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" run examples/path_budget.uc --max-steps 6
```

Seven steps allow the value 42 to be returned. Six exhaust the budget before RETURN. The failed run does not publish a successful result. The budget command accepts no --max-steps option: it measures the graph, whereas run applies a caller-selected limit.

## 5. Analyze an existing bytecode file

```powershell
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" compile examples/path_budget.uc path_budget.umcb
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" budget-bytecode path_budget.umcb
```

The stored artifact is decoded and validated before analysis. The report matches the source form when the artifact represents the same instructions. Neither budget command writes the input file. A damaged file, unsupported opcode, backward target or invalid stack/local state produces an error rather than an optimistic estimate. Existing inspect commands retain their output format.

## 6. Use the service from a C host

Include umicom_c/execution_bounds.h and link the shared Umicom C runtime. Supply an immutable valid instruction array, a diagnostic and a separate output object:

```c
UmcExecutionBounds bounds = {0};
UmcDiagnostic diagnostic = {0};
UmcStatus status = umc_execution_bounds(program, count, &bounds, &diagnostic);
if (status == UMC_OK) {
    /* Offer bounds.maximum_instructions as a proposed fuel allowance.
       The caller still chooses whether to execute the program. */
}
```

Here program and count denote the caller's decoded array and its instruction count, not a pointer into unvalidated file bytes. On failure the bounds object retains its previous fields. The diagnostic follows the existing validator. Inputs and outputs must be valid, disjoint objects; the program must not change during the call. No pointer is retained and no allocation, callback or host I/O occurs.

## 7. Understand the algorithm and storage

All admitted jumps point forward. Start at the final RETURN, whose remaining cost is one, then walk backward. An ordinary instruction adds one to the next record's cost. An unconditional jump adds one to its target's cost. A conditional adds one to the smaller or larger successor cost for the minimum or maximum. Each successor has already been measured, so recursion and repeated graph searches are unnecessary.

Two fixed automatic size_t arrays hold the intermediate costs. At the current 256-record limit, this is 4096 bytes when size_t is eight bytes, before other function/validator stack use. A constrained caller must account for the complete call chain. No successful route can exceed the admitted record count because backward edges are refused.

## 8. Keep the limits explicit

A program that divides by zero can still have a perfectly valid path count. Arithmetic execution may fail before reaching the minimum structural length. The maximum is sufficient fuel for a fresh execution of a valid program, but cannot promise arithmetic success. It also does not estimate a resumed machine's remaining fuel, real-time deadlines, application memory use or full C23 behavior.

The service reuses the existing complete validator. It does not add loops, new opcodes or a new wire version. The compiler, runtime capability masks and serialization contracts remain separate. See [program inspection](program-inspection.md), [execution traces](execution-traces.md) and [interactive debugging](interactive-debugging.md) for adjacent tools.
