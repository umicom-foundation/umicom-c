![Umicom](assets/branding/umicom-logo.svg)

# Keep temporary names inside a conditional body

A scope is the part of a program in which a name can be used. Umicom C admits
initialized integer declarations inside braced if, else and else-if bodies.
The compiler removes their names from lookup when the body ends. This helps
keep intermediate calculations close to the branch that needs them.

## Run the complete example

Open the Umicom C checkout and use a configured hosted build. The commands
below assume its executable is in build/bin; substitute your actual build
folder when needed. Run each command separately and stop on errors.

```powershell
.\build\bin\umicom-c.exe check .\examples\branch_locals.uc
.\build\bin\umicom-c.exe run .\examples\branch_locals.uc --max-steps 10
.\build\bin\umicom-c.exe trace .\examples\branch_locals.uc
```

The example declares result outside a branch, then declares a constant answer
inside that branch and copies its value into result. Check should accept ten
instructions. Run should print Result: 42 and Instructions: 10. The const
qualifier adds no instruction. A budget of nine steps refuses execution without
publishing a result.

```c
int main(void) {
    int result = 0;
    if (1) {
        const int answer = 42;
        result = answer;
    }
    return result;
}
```

The value 1 chooses the body. Changing it to 0 in a separate practice file
skips the initializer and assignment. The resulting program returns 0 after
six executed instructions. Its compiled program still contains ten instructions:
compilation records both paths, while execution takes one path.

## Observe a scope error

1. Copy the example to a new practice file.
2. Replace the final return result with return answer.
3. Run check on that copy. Expect the diagnostic local must be declared and
   initialized before use, located at the final answer name.
4. Restore return result. The outer value is available after the branch.

The compiler checks the whole source. Even an unselected body must use valid
names and must not assign to a const local. Arithmetic errors such as division
by zero occur only if the corresponding expression executes.

## Understand name lookup and slot reuse

A symbol table stores compiler information about declared names: their source
spans, local slots and read-only status. Before entering a body, the compiler
remembers the number of visible outer names. Each declaration first compiles
its initializer, then adds its name. On leaving the body, the compiler restores
the remembered count. That restoration also occurs if parsing fails.

This ordering refuses self-reference, duplicate declarations and use before
declaration. An inner body can read an earlier outer value. It cannot declare
another local with the same currently visible name: shadowing is deliberately
outside this bounded grammar. Separate sibling bodies may use the same name
because neither name remains visible while the other body is parsed.

```c
int main(void) {
    int result = 0;
    if (0) {
        int value = 7;
        result = value;
    } else {
        const int value = 42;
        result = value;
    }
    return result;
}
```

A local slot is a runtime location for one signed 32-bit integer. The compiler
can reuse a slot after its old name leaves scope. Scope exit does not clear
the slot, free heap storage or erase secrets. Source names are not encoded
into runtime bytecode. The runtime checks valid slot reads independently;
it cannot reconstruct the source-level name or const rules from wire records.

There are at most 32 simultaneously visible locals, including outer names.
Several disjoint bodies can declare more than 32 names across the complete
source while staying within that bound. The 256-instruction program limit,
source-size limit and shared parsing-depth limit still apply. Reusing slots
does not bypass any of those limits.

## Compare source and bytecode execution

Choose a new output filename: compile refuses to overwrite an existing file.

```powershell
.\build\bin\umicom-c.exe compile .\examples\branch_locals.uc .\branch-local-practice.umcb
.\build\bin\umicom-c.exe exec .\branch-local-practice.umcb --max-steps 10
.\build\bin\umicom-c.exe trace-bytecode .\branch-local-practice.umcb
.\build\bin\umicom-c.exe profile
```

This example uses canonical wire format 4, with the existing local and forward
branch operations. No new opcode or runtime capability is required. The
compiler advertises UMC_FEATURE_BRANCH_LOCALS, bit 1048576; the runtime does
not advertise this source-only feature. Other operators may require a newer
wire format. A new header does not upgrade an older linked compiler.

## Limits and verification

<!-- Earlier grammar retained for review:
Declarations require initializers. Standalone nested blocks, early returns,
loops, pointers, arrays and full C23 declaration rules remain unsupported.
-->
Declarations require initializers. Standalone nested blocks are also supported
through the same lexical owner; see [compound statements](compound-statements.md).
Early returns, loops, pointers, arrays and full C23 declaration rules remain unsupported.
An integer const local is a source update restriction, not a new C type system.

The regression source tests/test_branch_locals.c covers successful values,
escaped names, nested visibility, reused slots, maximum visible names, bytecode
roundtrips, fuel, runtime errors, exact source spans and depth refusal. Native
C computes independent expected results for small values with defined
arithmetic. tests/check_cli_branch_locals.cmake exercises the public commands,
refuses an escaped name without writing an artifact, compares traces and checks
that existing source and bytecode files remain unchanged.

If a command is unavailable, check the selected executable path and profile.
If a source is refused, first read the line and column in its diagnostic; a
scope error is not evidence that more VM memory is required.
