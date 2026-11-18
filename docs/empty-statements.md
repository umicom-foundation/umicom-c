![Umicom](assets/branding/umicom-logo.svg)

# Recognize an empty statement without inventing a VM operation

A semicolon on its own is an **empty statement**. It occupies source text but performs no work. Umicom C accepts it wherever the existing statement lists occur: before the required final return, inside braced conditional/standalone bodies, and after the final return before the function’s closing brace. The final return must still be the last nonempty statement.

~~~c
int main(void) {
    ;
    { ;; }
    return 6 * 7;
    ;;
}
~~~

The result is 42. The emitted program contains four instructions: PUSH 6, PUSH 7, MULTIPLY, RETURN. No NOP instruction is introduced. Empty statements do not allocate a local slot, consume VM fuel, increment expression depth or change the selected wire format. Their text still counts against the source-byte limit and takes bounded lexer work. A brace pair still consumes the normal structural nesting allowance.

## Try the actual source file

Use the build directory from your current successful Umicom C build. The commands below use an example hosted build layout; adjust it if yours differs. Run separately and stop on errors.

~~~powershell
Set-Location "C:\umicom\umicom-c"
.\build\bin\umicom-c.exe check examples/empty_statements.uc
.\build\bin\umicom-c.exe run examples/empty_statements.uc --max-steps 4
.\build\bin\umicom-c.exe run examples/empty_statements.uc --max-steps 3
.\build\bin\umicom-c.exe compile examples/empty_statements.uc build/empty-statements.umcb
.\build\bin\umicom-c.exe exec build/empty-statements.umcb --max-steps 4
.\build\bin\umicom-c.exe trace-bytecode build/empty-statements.umcb
~~~

Check reports four instructions. Run with fuel four prints Result: 42 and Instructions: 4. The intentional fuel-three exercise must fail without publishing a result; continue only after recognizing this expected refusal. Compile creates a new bytecode file and refuses an existing destination. Choose another output filename when repeating the exercise. Source and decoded execution have the same outcome. Trace offsets still point into the original source, including text displaced by extra semicolons.

## Keep grammar boundaries visible

Empty statements do not enable general expression statements, early returns, loops or unbraced conditional bodies. These inputs remain unsupported:

~~~c
int main(void) { ;;; }                 /* Missing return. */
int main(void) { return ; }            /* Missing return expression. */
int main(void) { if (1); return 42; }  /* This subset requires braces. */
int main(void) { if (1) {}; else {} return 42; } /* else is separated. */
int main(void) { 1 + 2; return 42; }   /* General expression statement. */
~~~

In the fourth example, the extra semicolon is a separate statement between the if and else. It must not be silently attached to either branch. The compiler continues checking both conditional bodies, even when a condition is a constant. A valid empty statement cannot hide an invalid declaration, out-of-scope name or missing delimiter.

## Inspect the source-only capability

UMC_EMPTY_STATEMENT_PROFILE is one. UMC_FEATURE_EMPTY_STATEMENTS is bit 16777216 of the compiler’s feature mask. Query umc_compiler_profile(UMC_PROFILE_FEATURES) and test that bit. The runtime must not advertise it: runtime-only consumers never parse semicolons. Existing opcode identities, runtime capabilities and canonical wire formats are unchanged. A new header does not establish that an older linked compiler supports this grammar.

## Read failures and warnings honestly

A failed compilation returns count zero; partial instruction storage may already have been written and must not be executed. The optional source-warning service still reports a never-read local surrounded by empty statements. Empty statements have no standalone warning in this implementation. Token locations remain byte-based, with CRLF treated as one line break.

A source program with thousands of semicolons can emit only two instructions for return 42. That illustrates why source bytes, parser depth, instruction capacity and runtime fuel are different resource limits. Never use a small instruction count as proof that an arbitrarily large source input is cheap to parse.

See [the language grammar](language.md), [capability queries](capability-profiles.md) and [source warnings](source-warnings.md). This remains a bounded C-shaped language, not complete C23 support.
