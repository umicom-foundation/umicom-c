<!--
Umicom C
File: docs/browser-integers.md

PURPOSE:
  Teach native signed integer artifact execution in a browser worker.

AUTHOR AND ORGANISATION:
Sammy Hegab
Umicom Foundation

LICENCE:
MIT
-->

![Umicom](assets/branding/umicom-logo.svg)

# Run checked integer operators in a browser worker

The worker's `integers` profile accepts native Umicom C formats 1 through 7.
Format 5 adds signed remainder, format 6 adds binary bitwise operators, and
format 7 adds checked shifts. Existing browser profiles still admit their
declared versions. A program uses the lowest format that supports all its stored
instructions, including instructions on paths that execution may skip.

1. Save this source as `integer-answer.uc`.

   ```c
   int main(void) {
       int value = 75 % 31;
       if (value > 0) {
           value = ((value << 2) ^ 7) - 9;
       }
       return value;
   }
   ```

2. Publish a new bytecode file, inspect its format and run the native VM.

   ```powershell
   umicom-c compile integer-answer.uc integer-answer.umcb
   umicom-c inspect-bytecode integer-answer.umcb
   umicom-c exec integer-answer.umcb --max-steps 64
   ```

   The remainder is 13. Shifting left two positions produces 52, XOR with 7
   produces 51, and subtracting 9 produces 42. Expect format 7 and result 42.

3. Set up the [browser worker](browser-arithmetic.md), fetch the same artifact
   as an `ArrayBuffer`, and transfer it with the integer profile selected.

   ```javascript
   worker.postMessage({ id: 7, profile: "integers", artifact, fuel: 64 }, [artifact]);
   ```

   Expect the native result and executed-instruction count. Sending the same
   format-7 program to the `branches` profile returns `unsupported-profile`.
   Fetch a fresh buffer before resending because transfer gives ownership to the worker.

## Check a runtime refusal

Use `int main(void) { return 1 << 31; }` in a separate artifact. It compiles
because the source is valid. The native and browser runtimes both refuse it
because the result exceeds the signed 32-bit range. The browser reports
`arithmetic-error` at the shift's source byte offset.

Shift inputs must be nonnegative and counts must be between 0 and 31.
Remainder refuses a zero divisor and the minimum integer divided by −1.
Binary AND, XOR and OR operate on 32 bits and interpret the result as a signed
integer. The worker validates the complete artifact, then executes only the
selected forward path within its fuel limit.
