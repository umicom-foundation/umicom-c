<!--
Umicom C
File: docs/short-circuit.md

PURPOSE:
Compile guarded integer expressions and distinguish structural paths from executed instructions.

AUTHOR AND ORGANISATION:
Sammy Hegab
Umicom Foundation

LICENCE:
MIT
-->

![Umicom](assets/branding/umicom-logo.svg)

# Guard an expression with short-circuit evaluation

A logical condition sometimes makes another expression unnecessary. In Umicom C, a false left operand of && skips the right operand. A true left operand of || also skips the right operand. Both operators return an integer zero or one.

## 1. Read the example

The complete source is [examples/short_circuit.uc](../examples/short_circuit.uc):

~~~c
int main(void) {
    int divisor = 0;
    return divisor != 0 && 42 / divisor > 1;
}
~~~

The comparison is false, so division is never attempted. This is not a general exception handler. Writing 42 / divisor > 1 first would fail before the guard could help. Both operands must still be syntactically valid and their bytecode structurally valid.

## 2. Build and inspect

Use the [execution guide](execution-guide.md) to configure and build the selected checkout. From its root, run each command separately and stop on unexpected errors:

~~~powershell
.\build\windows-ucrt64-debug\bin\umicom-c.exe check .\examples\short_circuit.uc
.\build\windows-ucrt64-debug\bin\umicom-c.exe inspect .\examples\short_circuit.uc
.\build\windows-ucrt64-debug\bin\umicom-c.exe trace .\examples\short_circuit.uc --max-steps 8
~~~

The expected compiled count is 16. The false path executes eight instructions and returns zero. Trace indices are 0, 1, 2, 3, 4, 5, 14 and 15. Indices 6–13 belong to the skipped arm. Inspection counts all 16 records because it checks both structural paths; it does not predict which path will execute.

Try the same trace with --max-steps 7. Expect an exhausted RETURN at index 15 and no completed result. A record index is not the amount of fuel already spent.

## 3. Understand the lowering

The compiler emits JUMP_IF_ZERO after the left operand. This consumes the condition. For &&, the false arm pushes zero; the true arm evaluates the right operand and compares it with zero. An unconditional JUMP skips the other arm. For ||, the true arm pushes one and the false arm evaluates the right operand. Patching records absolute instruction indices after their targets are known.

Equality binds more tightly than &&, which binds more tightly than ||. Parentheses can override this. There are no bitwise operators, loops, if statements or assignment expressions in this profile.

## 4. Transport the program

~~~powershell
.\build\windows-ucrt64-debug\bin\umicom-c.exe compile .\examples\short_circuit.uc .\short-circuit.umcb
.\build\windows-ucrt64-debug\bin\umicom-c.exe disassemble .\short-circuit.umcb
.\build\windows-ucrt64-debug\bin\umicom-c.exe exec .\short-circuit.umcb --max-steps 8
~~~

Choose a new output path; the tool refuses an existing artifact. Any branch requires canonical format 4. Formats 1–3 keep their previous byte encodings. An older runtime must be rebuilt or replaced by a matching SDK; changing the version byte does not convert a program.

## 5. Check all incoming paths

A branch target must be strictly later than its own record and inside the program. Every record must be structurally reachable. At a join, incoming stack depths must match. Initialized-local sets are intersected: a local written on only one arm cannot be read after the join. The validator does not infer that a constant condition makes another arm impossible.

These rules permit one bounded forward walk. They do not permit backward jumps, recursion or unbounded execution. The caller still owns disjoint, immutable input and separate result/trace storage.

## 6. Qualify the contracts

~~~powershell
ctest --test-dir .\build\windows-ucrt64-debug -R "umicom[.]c[.](logical[.]|cli-short-circuit)" --output-on-failure --no-tests=error
~~~

Source tests cover precedence and evaluated versus skipped faults. Runtime-only tests cover malformed joins, local initialization, exact literal format-4 bytes and fuel at each path position. The native comparison test stringizes the same program bodies that the host C compiler compiles; its corpus stays within defined shared behavior. Passing these tests does not establish full C compatibility. Kernel and Framework consumers require separate qualification against matching runtime sources.

If a trace contains the supposedly skipped divide, check operand order and parentheses first. If compilation succeeds but decoding rejects format 4, inspect library and header identity. If a handwritten program fails admission, inspect both incoming stack states and initialization sets; do not weaken the checks to accept it.
