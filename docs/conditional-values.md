<!--
Umicom C
File: docs/conditional-values.md

PURPOSE:
Select integer fallback values through the existing bounded branch runtime.

AUTHOR AND ORGANISATION:
Sammy Hegab
Umicom Foundation

LICENCE:
MIT
-->

![Umicom](assets/branding/umicom-logo.svg)

# Choose an integer value with a conditional expression

A condition can choose a value instead of merely producing true or false. The conditional operator has three parts: condition ? when_true : when_false. A nonzero condition selects the first arm; zero selects the second. Only the selected arm executes.

## 1. Read the supplied program

[conditional_value.uc](../examples/conditional_value.uc) contains:

~~~c
int main(void) {
    int divisor = 0;
    return divisor != 0 ? 42 / divisor : 7;
}
~~~

The zero divisor selects the fallback seven. Division is skipped. With divisor set to six, the first arm is selected and the result is also seven, but the execution path differs. Identical results do not establish identical operations or traces.

## 2. Build and run the selected checkout

Follow [the build guide](execution-guide.md), then run each command separately from the repository root:

~~~powershell
.\build\windows-ucrt64-debug\bin\umicom-c.exe profile
.\build\windows-ucrt64-debug\bin\umicom-c.exe check .\examples\conditional_value.uc
.\build\windows-ucrt64-debug\bin\umicom-c.exe trace .\examples\conditional_value.uc --max-steps 8
~~~

The current compiler feature mask is 1767, including conditional bit 1024. Runtime features remain 411. The example compiles to 12 records and follows an eight-instruction false path. Expect result seven and trace indices 0, 1, 2, 3, 4, 5, 10 and 11. A budget of seven exhausts at RETURN without publishing a completed result.

## 3. Understand grouping and values

Logical OR binds before the conditional operator. Thus 1 || 0 ? 7 : 9 selects seven. Nested conditionals associate to the right: a ? b : c ? d : e means a ? b : (c ? d : e). Both the middle and final arms may contain another conditional. Add parentheses for readability.

The selected integer is not normalized to zero or one. For example, 1 ? -9 : 42 returns minus nine. Logical && and || still normalize their results. Assignment remains a statement; comma expressions, omitted middle arms, pointers, implicit type conversions and if statements are unsupported.

Both arms must be valid source. A missing name in a skipped arm is still a compile error. A structurally valid division by zero in that arm is harmless only while the arm remains unselected.

## 4. Reuse the existing runtime

The compiler emits JUMP_IF_ZERO after the condition. The true arm produces one value and JUMP skips the false arm. The false arm also produces one value, so both paths join at the same stack depth. The existing VM admission checks all records before executing one path.

No new runtime opcode or public structure is needed. Conditional programs use canonical format 4, as logical short-circuit programs already do. A runtime accepting valid format-4 branches need not know the source operator spelling.

## 5. Respect parser bounds

Each recursive arm consumes one depth level, sharing the existing bound with unary nesting and parentheses. Tests cover 63 nested conditionals within the 256-record instruction limit and reject a 64th level. Other expressions can exhaust instruction capacity or recursion earlier. A rejected compile publishes count zero; never execute partial output slots.

## 6. Qualify and recover

~~~powershell
ctest --test-dir .\build\windows-ucrt64-debug -R "umicom[.]c[.](conditional[.]|cli-conditional|logical[.]logical_native)" --output-on-failure --no-tests=error
~~~

The source tests compare fixed values, selected arithmetic faults, syntax refusals and recursion boundaries. An independently enumerated wire fixture checks source offsets and branch targets. The native-C corpus uses the same defined program bodies without sharing VM implementation code.

If a wire fixture differs only in source offsets, enumerate the exact input bytes, including spaces. Do not change the compiler to fit an incorrectly counted fixture. If an old tool rejects ?, select the matching build and check its feature bit. Framework previews and Kernel consumers need their own source/SDK identity evidence; host success alone does not qualify them.
