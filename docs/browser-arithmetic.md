# Run Umicom C integer programs in a browser worker

![Umicom](assets/branding/umicom-logo.svg)

Umicom C compiles arithmetic to its own portable bytecode. A file ending in
`.umcb` contains those instruction bytes. The browser module reads format 1,
which covers integer constants, addition, subtraction, multiplication,
division, negation and return. The compiler and native VM use C23. This host
uses JavaScript because standard browser workers load JavaScript modules.
The opt-in comparison profile adds the six signed comparisons in format 2.

## Compile a practice program

1. Save the following function as `answer.uc`.

   ```c
   int main(void) {
       return (20 + 1) * 2;
   }
   ```

2. Compile the function to a new file and run it with the native VM.

   ```powershell
   umicom-c compile answer.uc answer.umcb
   umicom-c exec answer.umcb
   ```

   Expect `42`. Choose a fresh output name if the artifact already exists.

3. Serve `answer.umcb` and the `browser` directory from the same origin as the
   host page. An origin combines its scheme, host and port. For local practice,
   use a loopback web server that serves `.mjs` as JavaScript. A page opened
   directly from disk may refuse to load a module worker.

## Send the file to the worker

Put this code in your page's JavaScript module. A worker is a separate
execution thread with a message interface. You can use your existing page
controls to start the operation and show the returned integer.

```javascript
const worker = new Worker(new URL("./browser/worker.mjs", import.meta.url), {
    type: "module",
});
worker.addEventListener("message", ({ data }) => {
    console.log(data);
    worker.terminate();
});
const response = await fetch("./answer.umcb");
if (!response.ok) throw new Error("Could not load answer.umcb");
const artifact = await response.arrayBuffer();
worker.postMessage({ id: 1, artifact, fuel: 32 }, [artifact]);
```

The reply includes the same request ID, `ok: true`, value `42` and the
number of instructions executed. The transfer hands the buffer to the worker.
You can also obtain the buffer from a user's selected file with
`await file.arrayBuffer()`.

Use a budget from 0 through 256. A rejected run includes a reason and the
source byte offset carried by the artifact when available. If you see
`unsupported-profile`, check the artifact format and the selected profile.
Choose arithmetic, comparisons, [initialized local integers](browser-locals.md)
or [forward branches](browser-branches.md). The [checked integer profile](browser-integers.md)
also supports signed remainder, bitwise operators and checked shifts.
If you see `fuel-exhausted`,
inspect the artifact with `umicom-c inspect-bytecode` and compare its
instruction count with your budget. If the browser rejects a module's media
type, check that the server sends `text/javascript` for `.mjs`.

The worker interprets Umicom bytecode directly. It requires no WebAssembly
module, remote execution service or host imports. Once the worker and artifact
have loaded, the calculation uses local memory.

## Opt into comparisons

Compile `int main(void) { return (3 <= 4) * 42; }` as a separate artifact.
The comparison pushes one if it is true or zero if it is false. The
multiplication here converts the true result to 42.

```javascript
worker.postMessage({ id: 2, profile: "comparison", artifact, fuel: 32 }, [artifact]);
```

The comparison profile also accepts earlier format-1 artifacts. Without the
option, a format-2 artifact returns `unsupported-profile`. Use the optional
local-integers profile for format 3, the branches profile for format 4, and
the integer profile for formats 5 through 7.
Allow a new buffer for every transferred request.

## Check parity with the native compiler

With a C23 toolchain and Node.js 20 or newer, enable the optional conformance
tests before building.

```powershell
cmake -S . -B build/browser-check -G Ninja -DUMICOM_C_BUILD_BROWSER_TESTS=ON
cmake --build build/browser-check --parallel 2
ctest --test-dir build/browser-check --output-on-failure --no-tests=error -R '^umicom-c[.]browser[.]arithmetic$'
```

The test compiles artifacts with the native CLI, checks arithmetic, signed
comparisons and binary validation, and sends messages through the production worker with Node
supplying its message interface. Browser delivery also depends on the host
page, media types and worker policy. Verify those in the browser you deploy to.
