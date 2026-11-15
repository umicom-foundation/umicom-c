![Umicom](assets/branding/umicom-logo.svg)

# Select checked updates with if and else

Umicom C can choose a group of updates with a braced **if** statement and an optional braced **else**. This is a small source-language extension over the existing checked branch runtime. It does not add native machine-code generation or make the language a complete C23 implementation.

## Build and inspect a first branch

Use a C23 host compiler and Ninja. Start in the Umicom C checkout, keep generated files in its build directory, and stop after any failed command.

~~~powershell
cmake -S . -B build -G Ninja -DUMICOM_C_BUILD_TESTS=ON
cmake --build build
.\build\bin\umicom-c.exe profile
.\build\bin\umicom-c.exe check .\examples\conditional_update.uc
.\build\bin\umicom-c.exe run .\examples\conditional_update.uc --max-steps 8
~~~

<!-- Earlier profile statement retained for review:
The compiler feature mask now includes **UMC_FEATURE_IF_STATEMENTS**, bit 4096. Its complete current value is 16103; the runtime mask remains 411 and the supported wire-version mask remains 30. Query individual bits in a consumer, because adding a later source feature can change the complete mask without changing runtime operations.
-->
<!-- Earlier capability snapshot retained for review: The compiler feature mask now includes **UMC_FEATURE_IF_STATEMENTS**, bit 4096. Its complete current value is 65255; the runtime mask is 16795 and the supported wire-version mask is 62. Query individual bits in a consumer, because adding a later source feature can change the complete mask without changing runtime operations. -->
The compiler feature mask now includes **UMC_FEATURE_IF_STATEMENTS**, bit 4096. Its complete current value is 130791; the runtime mask is 82331 and the supported wire-version mask is 126. Query individual bits in a consumer, because adding a later source feature can change the complete mask without changing runtime operations.

These complete masks include the separate [checked remainder](remainder.md) extension. Remainder adds opcode 18 and format 5; the example in this guide retains its existing execution and minimal wire format.

The example declares value as one, evaluates a zero condition, and executes the else update:

~~~c
int main(void)
{
    int value = 1;
    if (0) {
        value = 7;
    } else {
        value = 42;
    }
    return value;
}
~~~

Expect eleven accepted instructions, result 42 and eight executed instructions. Accepted instruction count includes both arms. Executed count includes only the path taken. The source's leading comment changes recorded byte offsets but not its operations.

## Follow the two paths

The compiler evaluates the condition once and emits JUMP_IF_ZERO. That instruction consumes the condition: zero selects its target, and any nonzero signed value continues into the true body. A JUMP after the true body skips the else body. Both bodies finish with the same operand-stack depth.

| Index | Operation | Purpose |
| --- | --- | --- |
| 0 | PUSH 1 | Initial value |
| 1 | STORE_LOCAL 0 | Initialize the local |
| 2 | PUSH 0 | Evaluate the condition |
| 3 | JUMP_IF_ZERO 7 | Choose the false body |
| 4 | PUSH 7 | True-body value |
| 5 | STORE_LOCAL 0 | True-body update |
| 6 | JUMP 9 | Skip the false body |
| 7 | PUSH 42 | False-body value |
| 8 | STORE_LOCAL 0 | False-body update |
| 9 | LOAD_LOCAL 0 | Read the result |
| 10 | RETURN | Publish completion |

The false path visits indices 0, 1, 2, 3, 7, 8, 9 and 10. Changing the condition to one selects nine executed instructions because the true path includes JUMP. A condition is an integer expression, so comparisons, logical operators and conditional values can be used inside the parentheses.

~~~powershell
.\build\bin\umicom-c.exe trace .\examples\conditional_update.uc
.\build\bin\umicom-c.exe compile .\examples\conditional_update.uc .\build\conditional-update.umcb
.\build\bin\umicom-c.exe disassemble .\build\conditional-update.umcb
.\build\bin\umicom-c.exe exec .\build\conditional-update.umcb --max-steps 8
~~~

Choose a new artifact filename; compile refuses to overwrite an existing one. These branches use canonical format 4, which the runtime already supported for logical and conditional expressions. No opcode numbers or earlier artifact bytes change. The compiler's tests include independently written literal format-4 bytes rather than accepting whatever the encoder happens to produce.

## Keep names and scope explicit

Declare every local before the branch that updates it. A body may contain assignments, compound assignments and nested braced if statements. It may be empty. It may not introduce a declaration, return early, contain a standalone nested block or omit its braces. A final return after all top-level statements remains required.

~~~c
int main(void)
{
    int amount = 6;
    int factor = 7;
    if (amount > 0) {
        amount *= factor;
        if (amount < 0) {
            amount = -amount;
        }
    } else {
        amount = 0;
    }
    return amount;
}
~~~

There is still one flat local-name scope. Restricting declarations to the top-level statement sequence avoids implying branch-local scope or permitting a new name to be read on a path that never initialized it. The runtime independently validates slot initialization over every structural path before execution.

The words **if** and **else** are now reserved whole keywords. Names such as **if_value** and **elsewhere** remain valid. A previous experimental profile could scan these words as ordinary identifiers; rename such locals when adopting this grammar. Standard C also reserves these keywords.

An else attaches to the preceding completed braced if. It can contain a braced body or another **if** condition, forming an ordered chain. The first selected body executes and skips all later conditions. A final else remains optional. Read [else-if chains](else-if-chains.md) for the shared depth limit and exact trace example.

## Separate compile-time admission from execution

Both arms must parse and reference declared locals, even if the condition is a constant. An unknown name in a skipped arm is a compilation error. Arithmetic in that skipped arm is not executed: **if (0) { value = 1 / 0; }** leaves value unchanged. Selecting that arm produces the runtime's checked arithmetic error.

An error in the condition itself prevents either body from executing. A failed update does not publish a completed result or an assignment snapshot. The caller must check the returned status; partial output instructions from a failed compile have count zero and must not be executed.

Try these changes separately:

1. Run the example with a budget of seven. Expect fuel exhaustion at RETURN and no completed result.
2. Replace the zero condition with one. The true path needs nine steps and yields seven; the old eight-step budget must fail.
3. Add an undefined name to the false body while selecting the true body. Compilation must still reject it.
4. Put a declaration inside either body. Expect a syntax error, even when that body is skipped.
5. Remove the braces or the final return. These forms remain unsupported.

## Bound nested parsing

Statement nesting and expression recursion share the existing depth allowance. Each if adds one level before its condition and body expressions are parsed. A deeply nested body cannot reset that budget by starting a new assignment. Parentheses, unary operators and conditional-expression arms continue to consume depth as before.

The nominal limit is 64 parse levels, with depth checked before a primary/unary expression is admitted. A chain of 63 simple nested if statements with literal conditions can compile; a 64th simple if or an extra unary level at the deepest assignment is refused. Program size, stack depth and local count are separate limits, so another bound may reject a larger example first.

The regression suite also checks 127 flat empty conditionals followed by a literal return: exactly 256 instructions. One additional flat conditional exceeds the program bound. Limits are checked rather than bypassed through dynamic allocation.

## Qualify source and consumers

~~~powershell
ctest --test-dir build -R "umicom.c.(if-statement|cli-if-statement|profile|cli-profile)" --output-on-failure --no-tests=error
ctest --test-dir build --output-on-failure --no-tests=error
~~~

The cases cover selected values, skipped and executed arithmetic faults, rejected syntax, non-terminated source spans, combined depth, instruction capacity, literal wire bytes, every fuel cutoff and 361 small comparisons with native C. The native oracle uses defined small integer operations and does not execute signed overflow.

Framework's preview adapter must be qualified against the matching linked compiler. Kernel integration checks the independently supplied bytecode in its freestanding runtime; a Kernel runtime pass alone does not establish compiler syntax support. Preserve compiler identity, configuration, actual test results and skipped coverage separately. Read [language grammar](language.md), [capability profiles](capability-profiles.md), [compound updates](compound-assignments.md) and [the regression source](../tests/test_if_statement.c) together.

<!-- Earlier complete compiler mask statements retained for review before source complement:
The compiler feature mask now includes **UMC_FEATURE_IF_STATEMENTS**, bit 4096. Its complete current value is 32487; the runtime mask is 16795 and the supported wire-version mask is 62. Query individual bits in a consumer, because adding a later source feature can change the complete mask without changing runtime operations.
-->
