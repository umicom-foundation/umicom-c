![Umicom](assets/branding/umicom-logo.svg)

# Name true and false conditions

Umicom C recognizes the complete lowercase words **true** and **false** as integer literal values 1 and 0. They work wherever an integer expression is accepted. This is an additive spelling feature of the bounded integer language, not an implementation of C23's complete Boolean type system. Typed bool declarations, casts and headers remain outside this profile.

## Run a first example

Open PowerShell in your Umicom C source checkout. Use the build directory that you have configured and built; the paths below use build. If you need the toolchain, begin with [the execution guide](execution-guide.md). Run commands separately and stop on an error.

~~~powershell
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe
cmake --build build
ctest --test-dir build -R "boolean-literals" --output-on-failure
& ".\build\bin\umicom-c.exe" check .\examples\boolean_literals.uc
& ".\build\bin\umicom-c.exe" run .\examples\boolean_literals.uc
~~~

The source example is:

~~~c
int main(void) {
    const int enabled = true;
    int ready = false;
    if (enabled && !ready) {
        ready = 42;
    }
    return enabled ? ready : 1 / 0;
}
~~~

The result is 42. enabled is an initialized read-only integer with value 1. ready begins as integer 0. The if condition is true, so ready becomes 42. The conditional return selects ready and never evaluates 1 / 0. The variable ready is still an integer; it is not automatically limited to 0 and 1. This deliberate distinction prevents the new spelling from pretending that a new storage type exists.

Source checking accepts both expression arms after validating their syntax and names. Arithmetic faults occur when an operation is executed. Replacing enabled's initializer with false makes the example choose division by zero. That run fails with the existing checked-arithmetic status and publishes no result. A source check alone is not a successful run.

## Whole words and reserved names

true and false are case sensitive and now reserved. A declaration such as int true = 1 is refused with the existing diagnostic **expected a local integer name**. Rename a pre-existing local called true or false to a descriptive identifier before using this grammar profile. No automatic rewrite occurs.

Longer names such as true_value, true0 and falsehood remain ordinary identifiers. True and FALSE are also ordinary names, not alternate literal spellings. They require declarations before use. Comments are still skipped by the existing lexer, so return true /* explanation */; is accepted. A quote, punctuation error or embedded NUL does not extend the keyword's spelling.

## Source spelling and runtime values

| Source expression | Result | Explanation |
| --- | --- | --- |
| true | 1 | Literal true value |
| false | 0 | Literal false value |
| !false | 1 | Logical negation of zero |
| true + true | 2 | Existing integer addition |
| -true | -1 | Existing checked integer negation |
| false && (1 / 0) | 0 | Right operand is not executed |
| true || (1 / 0) | 1 | Right operand is not executed |
| true / false | execution error | Division by zero when executed |

The compiler emits the same PUSH instruction as for numeric 1 or 0. A program containing only return true needs two executed instructions: PUSH and RETURN. It retains canonical wire format 1. The larger example uses existing forward branches and therefore canonical format 4. No new opcode, wire version, heap allocation or runtime Boolean object is added.

Instruction source offsets point to the first byte of the keyword, and encoding/decoding preserves them. Source warning collection treats these tokens as constants rather than reads of identifiers. A local initialized from true can still be reported as never read. As before, a lexical read in a skipped branch counts as a source read; the warning service is not path-sensitive optimization.

## Discover support before offering it in an editor

Use UMC_BOOLEAN_LITERAL_PROFILE to identify this header contract. Query **UMC_FEATURE_BOOLEAN_LITERALS**, bit 134217728, from umc_compiler_profile(UMC_PROFILE_FEATURES) to inspect the linked compiler. The runtime must not advertise this source-only bit. Do not compare a frozen whole-feature mask: other independent features can be added.

Framework's optional compiler adapter can use the same linked compiler service. A newer header alone cannot update an older installed compiler library. Installed SDK and Studio UI integration need their own matching-source qualification. This source addition does not claim that a Studio gutter or syntax theme already understands the keywords.

## Read a failure before changing code

If check rejects true as an undeclared name, verify that the running compiler is the one you just built and that its Boolean feature bit is present. If int true is refused, use another variable name. If a runtime expression fails, inspect the selected path with trace and review its instruction budget. Do not replace an arithmetic error with a hard-coded success value.

The regression groups cover native C expression comparisons, exact and longer keyword spellings, reserved declarations, nested scopes, source warnings, every incomplete source prefix, byte offsets, portable bytecode and exact fuel. The CLI check separately verifies source/artifact execution, trace equality and no-clobber behavior. These checks cover the bounded profile; they do not establish full C23 compatibility.
