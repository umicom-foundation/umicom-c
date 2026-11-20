<!--
Umicom C
File: docs/int-input.md

PURPOSE:
Teach one-input native and browser execution with a shared bytecode artifact.

AUTHOR AND ORGANISATION:
Sammy Hegab
Umicom Foundation

LICENCE:
MIT
-->

![Umicom](assets/branding/umicom-logo.svg)

# Supply a signed integer to a compiled entry

A host is the program that loads and runs bytecode. It can supply one signed
32-bit value to `int umicom_run(int input)`. A parameter is a named local value
that begins with the data the host supplied. The same compiled program can be
used for many different inputs.

## Compile once and run with two inputs

1. Build the Umicom C tool using the [execution guide](execution-guide.md).
2. From the source checkout, compile the example to a new filename.

   ```powershell
   umicom-c compile-input examples/checked-input.uc checked-input.umcb
   umicom-c exec-input checked-input.umcb 20 --max-steps 64
   umicom-c exec-input checked-input.umcb -5 --max-steps 64
   ```

3. Expect `Result: 42` and `Result: -10`. A positive input adds two after
   doubling. A nonpositive input takes the shorter path. Compare the reported
   instruction counts.
4. Try a budget of one step. The CLI reports `instruction budget exhausted`.
   Try `2147483648`. The CLI reports the signed input range error before
   reading the artifact. Use a new output filename if compilation reports
   that a destination exists.

## Run the same artifact in a browser worker

Use the local HTTP setup from [the browser exercise](browser-arithmetic.md).
Keep the worker and its interpreter module together. Read the artifact into an
`ArrayBuffer`, then send the signed `input` and an explicit profile.

```javascript
const response = await fetch("./checked-input.umcb");
if (!response.ok) throw new Error("Could not load the checked input artifact");
const artifact = await response.arrayBuffer();
worker.postMessage({ id: 1, profile: "int-input", input: 20, artifact, fuel: 64 }, [artifact]);
```

The reply contains `ok: true`, `value: 42` and its `instructions` count.
Fetch the artifact again or transfer a fresh copy for another request. A
missing, fractional or out-of-range input reports `invalid-request`.

## Follow the native binding boundary

The compiler writes flag 1 in byte 5 of the wire header. Its first two
instructions are `PUSH 0` and `STORE_LOCAL 0`. The input-aware reader admits
the complete bytecode and checks those two instructions. It replaces the
placeholder on the active run with the caller's value. The original file and
the canonical instruction-format version stay unchanged. The ordinary `exec`
command rejects the flag because the program requires this binding.

Native embedders use `umc_compile_int_input` and
`umc_bind_int_input_bytecode` from `umicom_c/int_input.h`. Pass the bound
instructions to the existing executor. The host continues to own file access,
user input and display. The guest receives one value and runs within the
existing source, stack and instruction bounds.
