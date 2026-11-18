![Umicom](assets/branding/umicom-logo.svg)

# Read a source error with its surrounding line

A diagnostic tells you where the compiler or preview runtime found a problem. The command line still prints the filename, line and byte column first. It now follows source errors and unused-local warnings with a bounded excerpt and a caret. A caret is the `^` character that points at the reported position.

## Try a runtime error

1. Create a file called `divide.uc` containing the following source. Use an ordinary text editor and save as UTF-8 without a byte-order mark.

```c
int main(void) {
    return 1 / 0;
}
```

2. From the Umicom C repository, run the built tool:

```powershell
& .\build\windows-ucrt64-debug\bin\umicom-c.exe run .\divide.uc
```

Use your actual configured build directory if it differs. The example path is not a promise that a build already exists. The command returns a nonzero exit status because dividing by zero has no valid result. The message identifies the division operator and prints its source line. The excerpt does not run the program again.

3. Change the divisor to 2 and the numerator to 84. Run the command again. A successful run prints `Result: 42`. A syntax check alone does not establish that a later execution cannot fail.

## Read bytes and display columns separately

Source columns count bytes, starting at one. Display columns count characters in the escaped excerpt, starting at zero internally. A tab occupies one source byte but appears as the two visible characters `\t`. A backslash appears doubled. A zero byte appears as `\x00`; an escape byte appears as `\x1B`. Bytes outside printable ASCII also use hexadecimal escapes. This intentionally exposes the bytes instead of guessing the terminal's Unicode glyph widths.

A Windows CRLF pair is one line break. LF and CR also delimit lines. A position between the two bytes of CRLF refers to the start of the following line, consistent with the existing location API. An error at the end of a file may point one position beyond the final character. A file ending with a line break has an empty final line.

Only 80 source bytes are shown. Three dots at an edge mean that more of the same line exists outside the window. The dots are display markers, not edits to the source. The output has a fixed upper bound even if a source line occupies the whole 65,536-byte input limit.

## Use the service from a compiler client

```c
#include "umicom_c/diagnostic_excerpt.h"

const char source[] = {'x', '\t', 'y'};
UmcDiagnosticExcerpt excerpt;
if (umc_diagnostic_excerpt(source, sizeof source, 2, &excerpt) == UMC_OK) {
    /* excerpt.text is x followed by visible backslash-t followed by y.
       excerpt.column is 3; excerpt.caret_column is 3. */
}
```

Link `UmicomC::Compiler`. The source is a borrowed span: the caller owns its bytes and keeps them stable during the call. A terminating NUL is unnecessary. Supply a valid output object that does not overlap the input span. The function does not allocate memory and does not keep a pointer to the source.

NULL arguments, a source span above the limit, or an offset beyond its length return `UMC_INVALID_ARGUMENT` without changing the output. An offset equal to length is allowed. Do not inspect an uninitialized output object after failure. The public header documents the source window bounds and both column conventions.

## Scope and recovery

The ordinary source check, run, trace, inspect, budget, compile and operation-comparison error paths use the shared presentation. The warning-aware check also uses it. The interactive debugger has its own presentation and is not changed by this addition. Bytecode-only commands do not display binary data as if it were source. File I/O messages and filenames retain their existing formatting; this service is not a general terminal-output sanitizer.

If a caret appears far from the column number, look for visible byte escapes or clipped edges. If non-ASCII text appears as several hexadecimal escapes, inspect the original file in a UTF-8 editor. Do not replace arbitrary bytes merely to hide a diagnostic. Fix the underlying source or unsupported input encoding deliberately.

Run focused regression coverage after building:

```powershell
ctest --test-dir .\build\windows-ucrt64-debug -R 'diagnostic-excerpt' --output-on-failure
```

These tests cover newline boundaries, every byte value, long-line windows, output preservation and CLI routing. They do not establish full C23 support, terminal font rendering or native code generation.
