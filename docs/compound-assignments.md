![Umicom](assets/branding/umicom-logo.svg)

# Update local values with checked compound assignments

Use `+=`, `-=`, `*=` and `/=` when a statement updates an existing integer. Umicom C resolves the name to a bounded local slot, loads its old value, evaluates the right expression, performs checked arithmetic and stores the new value. This extends the source grammar without adding a virtual-machine instruction or a wire format.

## Build and run one update

Start in the Umicom C repository with a C23 host compiler and Ninja on your path. Run each command separately and stop on errors. An existing build directory must belong to this checkout.

~~~powershell
cmake -S . -B build -G Ninja -DUMICOM_C_BUILD_TESTS=ON
cmake --build build
.\build\bin\umicom-c.exe profile
.\build\bin\umicom-c.exe check .\examples\compound_assignment.uc
.\build\bin\umicom-c.exe run .\examples\compound_assignment.uc --max-steps 8
~~~

<!-- Earlier profile statement retained for review:
The compiler profile includes bit 2048, `UMC_FEATURE_COMPOUND_ASSIGNMENTS`. The current complete compiler mask is 16103; runtime features remain 411 and supported wire versions remain mask 30. Test individual bits in an application instead of assuming future complete masks will stay fixed. A newer header does not add syntax to an older linked compiler.
-->
<!-- Earlier capability snapshot retained for review: The compiler profile includes bit 2048, `UMC_FEATURE_COMPOUND_ASSIGNMENTS`. The current complete compiler mask is 65255; runtime features are 16795 and supported wire versions use mask 62. Test individual bits in an application instead of assuming future complete masks will stay fixed. A newer header does not add syntax to an older linked compiler. -->
The compiler profile includes bit 2048, `UMC_FEATURE_COMPOUND_ASSIGNMENTS`. The current complete compiler mask is 130791; runtime features are 82331 and supported wire versions use mask 126. Test individual bits in an application instead of assuming future complete masks will stay fixed. A newer header does not add syntax to an older linked compiler.

These complete masks include the separate [checked remainder](remainder.md) extension. Remainder adds opcode 18 and format 5; the example in this guide retains its existing execution and minimal wire format.

The example is:

~~~c
int main(void)
{
    int value = 6;
    value *= 7;
    return value;
}
~~~

Expect eight accepted instructions, result 42 and eight executed instructions. The compiler's source offsets include the example's leading comment. The comment changes locations, not arithmetic meaning.

## Follow the existing runtime operations

| Record | Operation | Purpose |
| --- | --- | --- |
| 0 | PUSH 6 | Evaluate the initializer |
| 1 | STORE_LOCAL 0 | Initialize the first slot |
| 2 | LOAD_LOCAL 0 | Copy the old value |
| 3 | PUSH 7 | Evaluate the right operand |
| 4 | MULTIPLY | Check and calculate the product |
| 5 | STORE_LOCAL 0 | Replace the stored value |
| 6 | LOAD_LOCAL 0 | Read the return expression |
| 7 | RETURN | Publish the completed result |

The old value and the right expression each evaluate once. Only local integer names can appear on the left; this profile has no pointers, subscripts, function calls or increment operators. Do not generalize this lowering to a future expression such as `array[index++] += f()` without designing its evaluation and ownership rules.

~~~powershell
.\build\bin\umicom-c.exe trace .\examples\compound_assignment.uc
.\build\bin\umicom-c.exe compile .\examples\compound_assignment.uc .\compound-assignment.umcb
.\build\bin\umicom-c.exe disassemble .\compound-assignment.umcb
.\build\bin\umicom-c.exe exec .\compound-assignment.umcb --max-steps 8
~~~

Choose a new artifact filename: compilation refuses to overwrite an existing output. This example requires canonical format 3 because it uses local slots. If its right expression uses `&&`, `||` or `?:`, the existing forward branches require format 4. The source spelling alone does not select a new format.

## Understand precedence and failure

`x *= 2 + 3;` multiplies the old value by five. The complete expression after the compound token is the right operand. `x += x * 2;` uses the old x in that expression. A right-side conditional or logical operator keeps its usual selective evaluation: `x += 0 && 1 / 0;` adds zero without dividing.

The arithmetic operation itself does not short-circuit. `x *= 1 / 0;` fails even when x is zero. Signed overflow, division by zero and minimum-int divided by minus one return `UMC_ARITHMETIC_ERROR`. Division of negative integers truncates toward zero. Native C signed overflow is undefined behavior, so a native oracle must avoid executing those cases; test Umicom’s defined refusals separately.

On an arithmetic error the trace identifies the attempted operator. The assignment's store has not executed, and the caller's `UmcResult` remains unchanged. Local slots are private to that execution; the API does not publish a partial local-variable snapshot. A failed compile sets the instruction count to zero; partial instruction storage must never be executed.

## Try controlled refusals

1. Run the valid example with `--max-steps 7`. Expect budget exhaustion before RETURN and no completed result.
2. Change the initializer to 2147483647 and the update to `value += 1;`. Compilation succeeds, but execution reports checked overflow at the compound operator.
3. Try `int value += 1;`. A declaration requires a plain initializer; declare the value first.
4. Try `value + = 1;` or split the operator with a comment. The two characters must be adjacent. Whitespace around the complete token is allowed.
5. Try `return value += 1;`. Assignment is a statement, not an expression. Complete the update before returning.

<!-- Earlier contract retained for review: Remainder assignment, bitwise assignment, chained assignment and increment/decrement remain unsupported. -->
Remainder assignment and binary bitwise assignments are supported; chained assignment and increment/decrement remain unsupported. See [binary bitwise operators](bitwise.md) for the eager &=, ^= and |= statements. The 32-local, 63-byte identifier, 256-instruction, 64-value stack and shared parse-depth bounds still apply. Saving the old value occupies one stack position while the right side executes.

## Qualify the extension

~~~powershell
ctest --test-dir build -R "umicom.c.(compound-assignment|cli-compound-assignment|profile|cli-profile)" --output-on-failure --no-tests=error
ctest --test-dir build --output-on-failure --no-tests=error
~~~

The new groups check values, malformed statements, exact non-terminated source spans, capacity boundaries, source locations, all fuel cutoffs and a literal wire fixture. A separate loop compares 1,425 defined small updates against native C arithmetic. Existing arithmetic, comparison, local and branch tests must continue to pass. These tests qualify their recorded source and environment; they do not prove arbitrary language compatibility or an application GUI.

Read [local integers](local-integers.md), [language syntax](language.md), [capability queries](capability-profiles.md) and [the regression source](../tests/test_compound_assignment.c) together. Framework consumers and Kernel guests need their own qualification when the linked source changes.

An update can now occur inside a braced conditional body. Its arithmetic and output-preservation rules stay the same; statement nesting contributes to the expression depth budget. See [if and else updates](if-statements.md) before using deep branches.

<!-- Earlier complete compiler mask statements retained for review before source complement:
The compiler profile includes bit 2048, `UMC_FEATURE_COMPOUND_ASSIGNMENTS`. The current complete compiler mask is 32487; runtime features are 16795 and supported wire versions use mask 62. Test individual bits in an application instead of assuming future complete masks will stay fixed. A newer header does not add syntax to an older linked compiler.
-->
