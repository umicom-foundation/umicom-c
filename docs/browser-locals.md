<!--
Umicom C
File: docs/browser-locals.md

PURPOSE:
Teach initialized local state in the native-format browser worker.

AUTHOR AND ORGANISATION:
Sammy Hegab
Umicom Foundation

LICENCE:
MIT
-->

![Umicom](assets/branding/umicom-logo.svg)

# Run an Umicom C program with browser-local integers

An Umicom C program with local integers uses format 3. Its bytecode refers to
integer slots numbered 0 through 31. The browser worker checks every slot
number and requires a preceding store before any load. The slots belong to
one run and are released when it returns.

1. Save the following in `local-answer.uc`.

   ```c
   int main(void) {
       int value = 20;
       int doubled = value * 2;
       value = doubled + 2;
       return value;
   }
   ```

2. Compile, inspect and run the native artifact before copying it to a web
   fixture. Expect the result 42. Choose a fresh artifact filename for a rerun.

   ```powershell
   umicom-c compile local-answer.uc local-answer.umcb
   umicom-c inspect-bytecode local-answer.umcb
   umicom-c exec local-answer.umcb --max-steps 64
   ```

3. Use the [existing browser setup](browser-arithmetic.md). Load the artifact
   as an `ArrayBuffer` and select the locals profile when sending it.

   ```javascript
   worker.postMessage({ id: 3, profile: "locals", artifact, fuel: 64 }, [artifact]);
   ```

   The browser should return the same integer and instruction count. The
   transfer consumes the page's buffer. Allocate or fetch a new buffer before
   resending.

An older profile returns `unsupported-profile` for the format-3 artifact.
`bad-bytecode` means admission failed before the program started. Compare
the original file bytes and native inspection results before adjusting a
budget. The local profile accepts earlier arithmetic and comparison
formats. Branches and later instruction families remain separate.
