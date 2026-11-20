<!--
Umicom C
File: docs/browser-branches.md

PURPOSE:
Teach bounded conditional control flow in the Umicom C browser worker.

AUTHOR AND ORGANISATION:
Sammy Hegab
Umicom Foundation

LICENCE:
MIT
-->

![Umicom](assets/branding/umicom-logo.svg)

# Choose an execution path in a browser worker

Umicom C format 4 supports forward branches. A zero condition chooses the
false path. The worker checks every path before running any instruction.
When paths meet, their stack depths must agree, and a later load needs a
local that was initialized on every path into that point.

1. Save this program as `choose-answer.uc`.

   ```c
   int main(void) {
       int input = 21;
       if (input > 10) {
           input = input * 2;
       } else {
           input = 7;
       }
       return input;
   }
   ```

2. Create a fresh artifact and run the native version before testing a page.

   ```powershell
   umicom-c compile choose-answer.uc choose-answer.umcb
   umicom-c inspect-bytecode choose-answer.umcb
   umicom-c exec choose-answer.umcb --max-steps 64
   ```

   Expect 42. Inspection counts all stored instructions. Execution counts the
   instructions used by this input, including the branch decisions and return.

3. Use the [browser setup](browser-arithmetic.md). Load the artifact into an
   `ArrayBuffer` and request the branches profile.

   ```javascript
   worker.postMessage({ id: 4, profile: "branches", artifact, fuel: 64 }, [artifact]);
   ```

   Expect the same value and execution count as the native run. The older
   local-integers profile refuses format 4. Refresh the artifact buffer
   before sending it again.

If the verifier refuses a program, inspect the branch target and the state
on each incoming path. The target must refer to a later record in the same
program. A missing initialization cannot be repaired by increasing fuel.
Use the [integer profile](browser-integers.md) when a forward path contains
signed remainder, a binary bitwise operator or a checked shift.
