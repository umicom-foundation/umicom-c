<!--
Umicom C
File: docs/execution-traces.md

PURPOSE:
Read and embed bounded arithmetic traces with explicit output ownership.

AUTHOR AND ORGANISATION:
Sammy Hegab
Umicom Foundation

LICENCE:
MIT
-->
# Observe a bounded execution trace

A trace records completed instructions and, when execution fails, the next rejected instruction. Umicom C exposes the same executor to the CLI, hosted library consumers and freestanding user programs. This facility supports the existing arithmetic profile; it is not a native debugger, a breakpoint service or a full C23 implementation.

## 1. Prepare the current tool

Follow [the build guide](execution-guide.md) and rebuild the compiler from source containing [trace.h](../include/umicom_c/trace.h). Use the exact binary in that build directory. An older installed umicom-c may know run and inspect but not trace. Run each command separately and stop on errors.

```powershell
Set-Location "C:\umicom\umicom-c"
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" trace ".\examples\inspection.uc"
```

The existing example returns -(2 + 3 * 4). Expect seven successful rows with top values 2, 3, 4, 12, 14, -14 and -14, then Result: -14 and Instructions: 7. These are expectations to verify on your own build, not a recorded pass.

## 2. Read a row

| Field | Meaning |
| --- | --- |
| step | Zero-based instruction index |
| source-byte | Byte offset retained by compilation or decoding |
| opcode | Operation being attempted |
| stack-depth | Number of current stack values |
| top | Last stack value, or zero for an empty stack |
| status | Whether this instruction completed or was refused |

Successful rows show the state after the instruction. A failed row shows the unchanged state before the rejected instruction. Binary arithmetic checks its operands and range before publishing a changed value or depth. The trace stores the top value, not the complete stack.

## 3. Compare fuel and arithmetic failures

```powershell
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" trace ".\examples\inspection.uc" --max-steps 6
$LASTEXITCODE
```

Expect six successful rows and one rejected RETURN row, exit 1 and no Result line. The last top value is already -14, but the function has not completed. A trace row count is therefore not the number of successfully executed instructions.

For a separate example containing int main(void) { return 7 / 0; }, expect two successful pushes and a failed divide at depth 2 with top 0. No RETURN runs. Source diagnostics identify the byte location; malformed source produces no execution rows at all.

The CLI accepts --max-steps 1..256 for run, exec, trace and trace-bytecode. The library also accepts fuel zero and records its first rejected instruction with an empty stack. Validation still checks the whole program before execution; fuel is not a deadline for validation or a wall-clock timeout.

## 4. Keep bytecode and source identity separate

```powershell
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" compile ".\examples\inspection.uc" "trace-study.umcb"
& ".\build\windows-ucrt64-debug\bin\umicom-c.exe" trace-bytecode "trace-study.umcb"
```

Compilation requires a new output filename. Tracing does not change the bytecode format or add fields to it. Source and bytecode traces should agree when they describe the same compiled source. Comment or whitespace edits can move source offsets while preserving operations. Save the source identity alongside a trace instead of attaching old offsets to a newer document.

## 5. Embed the API

```c
UmcTraceStep steps[UMC_PROGRAM_LIMIT];
size_t used = 0U;
UmcStatus status = umc_execute_trace(program, count, fuel, &result,
    steps, UMC_PROGRAM_LIMIT, &used, &diagnostic);
```

The caller owns every object and keeps writable regions disjoint from the input and one another. The runtime validates the entire program and requires capacity >= count before executing. Argument, shape and capacity failures publish zero entries and leave the trace bytes and result unchanged. Runtime faults preserve completed entries and one attempted fault entry, while leaving result unchanged. Only inspect entries below used. No heap allocation, callback, file operation or retained pointer is introduced.

The existing umc_execute API uses the same executor without collecting entries. Framework's optional [preview adapter](https://github.com/umicom-foundation/umicom-framework/blob/main/include/umicom/toolchain/cvm_trace.h) adds fresh presentation state. Kernel's optional CVM program owns trace buffers inside its native user envelope. Those consumers require their own qualification.

## 6. Qualify the contract

```powershell
ctest --preset windows-ucrt64-debug -R "^umicom[.]c[.](trace[.]|cli-trace$)" --output-on-failure --no-tests=error
```

Runtime cases check successful rows, arithmetic boundaries, fuel, preflight refusal, buffer reuse and wire transport. The CLI case compares source and bytecode output, rejects malformed options and checks unchanged inputs. Keep skipped coverage and missing test selections separate from passes. A compiler or linker error involving trace.h means the source or SDK selection needs attention before runtime results can be assessed.
