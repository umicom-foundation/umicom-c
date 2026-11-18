![Umicom](assets/branding/umicom-logo.svg)

# Compute intermediate values with local integers

A local variable gives a name to one signed 32-bit value during a program execution. Umicom C supports initialized declarations and assignment statements in its single main function. This remains a deliberately bounded language, not a full C23 implementation.

## Build and run the example

From the Umicom C repository, use an existing C23 compiler and Ninja. Run each command separately and stop on an error.

~~~powershell
cmake -S . -B build -G Ninja -DUMICOM_C_BUILD_TESTS=ON
cmake --build build
.\build\bin\umicom-c.exe profile
.\build\bin\umicom-c.exe check .\examples\local_integers.uc
.\build\bin\umicom-c.exe run .\examples\local_integers.uc --max-steps 8
~~~

The profile reports local integers in both components, a 32-slot bound and a 63-byte source identifier bound. The check command accepts eight instructions. Execution reports Result: 42 and Instructions: 8.

The example's body is:

~~~c
int main(void)
{
    int value = 6;
    value = value * 7;
    return value;
}
~~~

Declaration creates the name after its initializer has compiled. Assignment replaces its stored value. Reading value does not consume the stored value: it pushes a copy onto the operand stack.

## Follow the lowering

~~~powershell
.\build\bin\umicom-c.exe trace .\examples\local_integers.uc
.\build\bin\umicom-c.exe compile .\examples\local_integers.uc local-integers.umcb
.\build\bin\umicom-c.exe disassemble .\local-integers.umcb
.\build\bin\umicom-c.exe exec .\local-integers.umcb --max-steps 8
~~~

Choose a new output filename. Compilation intentionally refuses to replace an existing file.

| Instruction | Operation | Stack after success |
| --- | --- | --- |
| 0 | PUSH 6 | [6] |
| 1 | STORE_LOCAL 0 | [] |
| 2 | LOAD_LOCAL 0 | [6] |
| 3 | PUSH 7 | [6, 7] |
| 4 | MULTIPLY | [42] |
| 5 | STORE_LOCAL 0 | [] |
| 6 | LOAD_LOCAL 0 | [42] |
| 7 | RETURN | [42] |

The slot number is assigned in declaration order. It is not a native memory address. Trace rows report stack depth and top; when depth is zero, top is reported as zero. This API does not expose a snapshot of all local slots.

## Diagnose rejected programs

1. Change the initializer to value itself. Compilation must reject the read because value has not been declared yet. Restore the initializer before continuing.
2. Try assigning to missing. The compiler rejects the undeclared name. Check spelling and declaration order; names are case sensitive.
3. Add a second declaration of value. It is a duplicate, even if its initializer differs. Assign to the existing name instead.
4. Run the valid example with --max-steps 7. The last return cannot execute. No result is published; a failed trace records that attempted step.
<!-- Earlier grammar retained for review:
5. Add 33 initialized names. The compiler returns UMC_LOCAL_CAPACITY and publishes an instruction count of zero. Split a larger design into a supported host application; this profile does not yet have functions or nested scopes.
-->
5. Add 33 initialized names in the same visible scope. The compiler returns UMC_LOCAL_CAPACITY and publishes an instruction count of zero. Separate independent calculations into distinct [lexical blocks](compound-statements.md) when their temporary values do not need overlapping lifetimes. Functions remain unsupported, and nested blocks do not bypass the program instruction limit.

<!-- Earlier grammar retained for review:
There is no uninitialized declaration, chained assignment, comma declaration, increment operator, array, pointer or standalone nested block. int x=1; is supported; int x; and return x=2; are not. An initializer may use earlier variables, arithmetic, comparison, ! and supported numeric spellings. Checked arithmetic errors still occur during execution.
-->
There is no uninitialized declaration, chained assignment, comma declaration, array or pointer. int x=1; is supported; int x; and return x=2; are not. Standalone increment/decrement statements and nested lexical blocks are supported. An initializer may read earlier visible variables using the documented expression operators. Checked arithmetic errors occur during execution; a const write or escaped name is refused during compilation.

## Understand the storage boundary

The compiler borrows source spans for names only during compilation. Its output contains numeric slots, never a pointer into source. The runtime owns an automatic 32-element array for each invocation. A complete validation pass proves that every LOAD follows a STORE to the same valid slot. An invalid index is rejected before either a shift in the initialization map or an array access.

Slots do not persist between calls. A program that successfully stores slot 31 cannot make another program's uninitialized load valid. Validation runs before fuel accounting or result publication. Input instructions must remain immutable during a call and all caller buffers must obey the API's disjointness contract.

## Qualify each consumer separately

~~~powershell
ctest --test-dir build -R "umicom.c.(locals|cli-local-integers|profile|cli-profile)" --output-on-failure --no-tests=error
ctest --test-dir build --output-on-failure --no-tests=error
~~~

Source tests cover visibility, syntax, identifier and slot bounds. Runtime-only tests use independent records and literal wire bytes, then check failed output preservation, every fuel boundary and repeat execution. Format 3 is required by local operations; earlier arithmetic and comparison encodings retain formats 1 and 2.

The Kernel CVM envelope has separate local selectors at O0 and O2. The optional Framework preview adapter translates local capacity to its shared capacity status and clears stale results on failure. Install matching headers and libraries before qualifying that consumer; host test success does not establish SDK, application or GUI qualification.

Read [the language](language.md), [wire layout](bytecode.md) and [capability queries](capability-profiles.md) alongside [test_local_source.c](../tests/test_local_source.c), [test_local_runtime.c](../tests/test_local_runtime.c) and [the CLI fixture](../tests/check_cli_local_integers.cmake).

## Update using the old value

Initialized locals also accept `value += amount;`, `value -= amount;`, `value *= factor;` and `value /= divisor;`. Declarations still need `=`. These statements retain checked arithmetic, per-execution slot storage and failed-result preservation. Follow [compound assignment lowering](compound-assignments.md) before changing a compiler consumer.

## Update along a selected path

<!-- Earlier scope restriction retained for engineering history:
A braced **if** body and its optional braced **else** body may assign to existing locals or nest another conditional. Declare names before these bodies; body-local declarations and early returns are not accepted. Both paths are compiled, but only the selected path executes its arithmetic. This preserves the flat name scope and existing definite-initialization rules. See [conditional statements](if-statements.md) for the branch layout, fuel differences and shared parse-depth budget.
-->
A braced **if** body and its optional braced **else** body may declare initialized locals, update visible names or nest another conditional. A new body-local name is visible only after its initializer and before that body ends. Early returns and shadowing an active name remain unsupported. Both paths are compiled, but only the selected path executes its arithmetic. See [conditional statements](if-statements.md) for branch layout and [branch-local scope](branch-local-scope.md) for name lifetime and bounded slot reuse.
