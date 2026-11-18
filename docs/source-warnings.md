![Umicom](assets/branding/umicom-logo.svg)

# Check local reads before execution

The optional source-warning service reports initialized local integers whose
values are never read in the supported source language. It uses the same parser
and bytecode validator as ordinary compilation. It does not execute the program,
run a native compiler, or introduce a second interpretation of the grammar.

## Try the command

1. Use a configured Umicom C build containing the hosted command. The paths below
   assume build/bin in the current checkout; substitute your actual build folder.
2. Open [source_warnings.uc](../examples/source_warnings.uc). The variable omitted
   is initialized but never read; answer is read by its compound update and return.
3. Run each command separately and stop on unexpected errors:

~~~powershell
.\build\bin\umicom-c.exe check .\examples\source_warnings.uc
.\build\bin\umicom-c.exe check .\examples\source_warnings.uc --warnings
~~~

Both commands should accept ten instructions. The second also reports one warning
at omitted's declaration. Line numbers include the file's licence header. The
warning is printed to stderr, and the acceptance summary to stdout. Display order
between streams is not guaranteed. Successful admission returns exit status zero,
including when warnings exist; source or usage errors return two. The old check
command retains its existing output and does not enable warnings automatically.

The option is specific to check. It does not make run, compile, inspect or the
interactive debugger accept a new execution policy. In particular, compile still
interprets its fourth argument as a new output path, as documented by that command.

## Define what counts as a read

A plain assignment such as x = 2 writes without reading the old x. Expressions
such as return x, x = x + 1 and another variable's initializer read x. Compound
updates and standalone prefix/postfix increment or decrement also read it.

A read in an unselected conditional or short-circuited expression counts. This is
lexical analysis: it observes the admitted source, not the path taken by one run.
It does not prove that a value affects the final result. A variable that is only
incremented is read by those increments and does not receive this warning.
Dead-store analysis and path-sensitive usefulness are separate, unimplemented checks.

Names in disjoint scopes have distinct declaration identities even when the VM
reuses a slot. The report is ordered by declaration position, not the order in
which braces close. Qualifying a local with const does not suppress the warning.
Active-name shadowing remains unsupported by the language itself.

## Call the compiler service

Include [source_check.h](../include/umicom_c/source_check.h) and link
UmicomC::Compiler. The existing runtime-only target does not acquire the parser
or report service. A newer header alone does not add the symbol to an older SDK.
Use headers and libraries from the same qualified installation.

~~~c
#include "umicom_c/source_check.h"

/* Inside a C function: */
const char source[] = "int main(void){int unused=7;return 42;}";
UmcSourceReport report;
UmcDiagnostic diagnostic;
UmcStatus status = umc_check_source(source, sizeof source - 1U,
    &report, &diagnostic);
if (status == UMC_OK) {
    /* report.warning_count == 1; report.instruction_count == 4. */
    /* Inspect only entries below report.warning_count. */
}
~~~

The report owns numeric positions, not a source pointer. It remains unchanged
when admission fails. The separate error diagnostic follows umc_compile's existing
contract. Report, diagnostic and source storage must not overlap. The caller must
provide a readable source span of the declared length and keep it stable during
the call. There is no allocator, retained pointer, background worker or file I/O.

Each warning contains a stable kind, zero-based source byte offset, name byte
length, and one-based line and byte column. A tab is one byte column, not a promise
about its drawn width. CRLF, CR and LF are located through the existing shared
routine. An editor must translate byte positions to screen positions and associate
the report with the exact source revision. An unchanged previous report after a
failed check is not evidence about the new text.

## Memory and publication

The optional collector uses bounded call-local arrays. Every declaration emits a
store, so the 256-instruction ceiling also bounds total declaration records. The
32-local limit concerns simultaneously visible names; disjoint scopes can reuse
those slots. Fixed arrays still consume stack space. Ordinary umc_compile passes
no collector and does not allocate the optional report arrays.

The checker finishes syntax and instruction admission before publishing a report.
No partially gathered warning list is published after a later error. On success,
UMC_WARNING_LOCAL_NEVER_READ identifies each reported declaration. Query compiler
feature bit UMC_FEATURE_SOURCE_WARNINGS to discover the linked implementation's
claim; the runtime does not advertise it. No opcode or wire format changes.

## Do not confuse acceptance with execution

An initializer containing 1 / 0 can pass source checking. It fails only when the
VM executes division. Even an unused initializer executes in the current language,
so removing its declaration can change observed behavior. Review a warning before
editing; never automatically delete a declaration merely to remove the message.

The service is not a complete C23 analyzer, ownership checker, race detector or
memory-leak detector. It is not yet connected to a Studio warning gutter. Framework
preview continues to compile and execute through its existing optional adapter.
These distinct operations should remain explicit in a product interface.

## Qualify the boundary

~~~powershell
ctest --test-dir .\build -R "source-warnings" --output-on-failure
~~~

The cases exercise all update forms, unselected reads, lexical slot reuse, source
ordering, line-ending conventions, every truncated prefix, exact instruction
capacity, overflow refusal, and repeated independent calls. Refusal checks compare
sentinel-filled report bytes before and after failure. Separate process checks
verify real CLI output, old-command compatibility and invalid argument rejection.
Use a configured and built test tree; missing executables are failures, not passes.
