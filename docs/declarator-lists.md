![Umicom](assets/branding/umicom-logo.svg)

# Declare initialized integers in order

A declaration introduces a name and its initial value. A declarator is the part that names one object. Umicom C accepts multiple initialized integer declarators after one type prefix:

~~~c
int main(void) {
    int first = 6, second = first * 7;
    return second;
}
~~~

This returns 42. The comma separates two declarators; the semicolon ends the whole declaration. Both names remain ordinary signed 32-bit locals. This is a bounded C-shaped language, not a complete C23 compiler.

## Read the declaration from left to right

1. int selects the supported integer declaration form.
2. first names the first local. Its initializer is 6.
3. The compiler emits the initializer and stores it in a local slot.
4. Only after that initializer has compiled does first become available to later source.
5. The comma begins another declarator, second = first * 7.
6. That initializer can read first. It cannot read second before its declaration completes.
7. The semicolon ends the list; return second reads its initialized value.

The same sequence could be written as int first = 6; int second = first * 7;. Its opcodes and operands are identical. Source offsets differ because the tokens occupy different byte positions. Byte-for-byte artifact equality is therefore not the appropriate comparison for differently formatted source.

## Qualifiers and scope

const int a = 6, b = a * 7; and int const a = 6, b = a * 7; make both names read-only in this source language. The prefix is consumed once for the whole list. A later b = 9; or ++b; is refused, including inside a branch that would not execute. Const is a compiler rule here, not a new immutable VM storage type.

Existing lexical scopes apply to every name. A nested block can read outer names, but it cannot declare a new name that is already visible. After a block ends, its local names disappear and later disjoint blocks may reuse their slots. Up to 32 local slots may be visible together. A comma does not create a new scope or enlarge that capacity.

## Every initializer remains required

| Source fragment | Result |
| --- | --- |
| int a = 1, b = a + 1; | Accepted; b becomes 2 |
| int a = 1, b; | Refused: b has no initializer |
| int a = 1, b = b; | Refused: self-reference |
| int a = b, b = 1; | Refused: forward reference |
| int a = 1, a = 2; | Refused: duplicate name |
| int a = 1,; | Refused: missing next declarator |
| int a = 1, int b = 2; | Refused: a second type prefix within the list |
| a = 1, b = 2; | Refused: comma assignments are not declaration lists |
| return (1, 2); | Refused: comma expressions are unsupported |

Declaration syntax is checked even in an unselected conditional body. Executed arithmetic is checked at runtime: int a = 1 / 0, b = 2; can pass source checking but fails when its first initializer executes. Later initializers are not executed after that failure. The completed-result output remains unchanged.

## Run the eight-instruction example

Use the existing Windows preset after selecting UCRT64. Run each command separately and stop on an unexpected failure. These commands are for the owner checkout after integrating the matching complete files.

~~~powershell
Set-Location "C:\umicom\umicom-c"
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
cmake --preset windows-ucrt64-debug
cmake --build --preset windows-ucrt64-debug --parallel 2
.\build\windows-ucrt64-debug\bin\umicom-c.exe check examples/declarator_lists.uc
.\build\windows-ucrt64-debug\bin\umicom-c.exe run examples/declarator_lists.uc --max-steps 8
~~~

Expect check to accept eight instructions and run to print Result: 42 and Instructions: 8. Their operations are PUSH 6, STORE first, LOAD first, PUSH 7, MULTIPLY, STORE second, LOAD second, RETURN. A budget of seven deliberately fails before RETURN can complete. This example uses the existing local-integer wire format 3; the comma introduces no instruction.

To exercise focused checks:

~~~powershell
ctest --test-dir build/windows-ucrt64-debug -R "umicom.c.(declarator-lists|cli-declarator-lists|profile|cli-profile)" --output-on-failure --no-tests=error
~~~

The tests compare separate and combined declarations, validate every qualifier, inspect warning order, exercise exact local/program bounds, reject truncated spans, round-trip the bytecode and verify first-failure behavior. A passing source check does not prove arithmetic can never fail.

## Reuse the existing compiler boundary

The parser consumes a declaration prefix, then invokes one declarator routine for each name. Each routine parses the initializer before publishing that name to source lookup, checks the existing warning capacity, and emits an ordinary STORE_LOCAL. An unexpected delimiter refuses the whole source operation. On failed compilation the published instruction count is zero; callers must not execute partly written instruction storage.

The warning collector uses the same lookup. In int a = 1, b = a + 1; return b;, a is read by b's initializer and should not be reported as never read. Warning order follows declaration order, independently of slot reuse.

Test UMC_FEATURE_DECLARATOR_LISTS in umc_compiler_profile(UMC_PROFILE_FEATURES). The runtime must not advertise this source-only bit. A newer header does not upgrade a linked compiler or a previously built application. Installed SDK and target-specific consumers require their own matching-source qualification.

See [the full supported grammar](language.md), [local storage](local-integers.md), [read-only names](readonly-locals.md) and [source warnings](source-warnings.md).
