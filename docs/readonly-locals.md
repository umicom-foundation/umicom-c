![Umicom](assets/branding/umicom-logo.svg)

# Catch accidental writes to fixed local values

A local variable is a name for a value held during one execution. Some values
should stay fixed after initialization. Umicom C accepts `const int` and
`int const` to express that rule in its bounded integer source language.
The compiler checks the rule before producing executable bytecode.

## Run a small example

1. Open the Umicom C checkout in PowerShell. Use a configured hosted build
   containing the current compiler and runtime.
2. Read `examples/readonly_locals.uc`. `unit` captures six and `count` captures
   seven. Their product initializes an ordinary mutable local named `total`.
3. Run the commands below separately. Stop if a command fails. Substitute your
   actual build directory if it differs from `build`.

```powershell
.\build\bin\umicom-c.exe check .\examples\readonly_locals.uc
.\build\bin\umicom-c.exe inspect .\examples\readonly_locals.uc
.\build\bin\umicom-c.exe run .\examples\readonly_locals.uc --max-steps 10
```

The check should accept ten instructions. Execution should print `Result: 42`
and `Instructions: 10`. A budget of nine steps is insufficient and reports
exhaustion without publishing a result. The qualifier itself adds no VM
instruction. It is checked in the compiler symbol table.

```c
int main(void) {
    const int unit = 6;
    int const count = 7;
    int total = unit * count;
    return total;
}
```

## Compare initialization with assignment

Initialization supplies the first value when a declaration executes. Assignment
changes a previously initialized variable. The required initializer is still
an ordinary expression: it can read earlier locals and can fail at runtime.

```c
int main(void) {
    int input = 6;
    const int captured = input * 7;
    input = 0;
    return captured;
}
```

This returns 42. `captured` stores a value; it is not a live formula that follows
`input`. Its initializer runs before its own name becomes visible, so
`const int x = x;` is refused. A later declaration is also unavailable to an
earlier initializer. No constant folding or compile-time evaluation is promised.

## Diagnose a rejected write

In a separate practice file, add `unit = 8;` before the final return. Run
`check` again. Expect a syntax diagnostic containing `cannot update a read-only
local integer`, positioned at the attempted update name. Remove that line
after observing the refusal.

The same rule covers `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `^=`, `|=`, `++` and
`--`. Both prefix and postfix statement forms are checked. Even
`if (0) { unit++; }` is refused: both source branches must satisfy the language
contract, although only the selected branch executes. A failed source compile
does not publish a bytecode output file. The low-level compiler API retains its
existing rule that the instruction count is zero on failure; callers must not
execute partial output storage.

## Keep the supported grammar explicit

Only one `const` qualifier is accepted, immediately before or after `int`.
<!-- Earlier scope restriction retained for engineering history:
An initializer is required, one declaration names one local, and declarations
still occur only at the top level before the final return. Repeated qualifiers,
conditional-body declarations, pointers, arrays, reference types, `constexpr`,
and a qualified `main` return type are not implemented by this extension.
-->
An initializer is required and one declaration names one local. Declarations
may occur in the top-level statement sequence or a conditional body; body names
follow [lexical scope](branch-local-scope.md). Repeated qualifiers, pointers,
arrays, reference types, `constexpr` and a qualified `main` return type are
not implemented by this extension.
`const` is now a reserved keyword; longer names such as `constant` remain
identifiers. Old source using the exact name `const` must choose another name.

The existing signed 32-bit range, source-span limit, recursion limit and
32-local capacity still apply. A read-only local occupies one normal slot.
`const int ratio = 1 / 0;` passes grammatical admission but fails during
execution. Read-only qualification does not establish arithmetic validity.

## Understand the bytecode boundary

The existing LOAD_LOCAL and STORE_LOCAL instructions remain unchanged. A simple
read-only-local program uses canonical bytecode format 3, just like the
equivalent mutable-local program. Source offsets can differ because the source
spelling is longer. Encoding and decoding retain those offsets.

The bytecode format does not record the qualifier. A separately authored
bytecode program can still store to a valid initialized local under the runtime
contract. Do not describe source `const` as a security boundary, read-only
operating-system page or guarantee against bytecode modification.

`umc_compiler_profile(UMC_PROFILE_FEATURES)` reports
`UMC_FEATURE_READONLY_LOCALS`. The runtime profile does not report that
source-only bit. A new header alone cannot add the capability to an older
library. Query the linked implementation and keep matching source identities
when debugging an embedded compiler.

## Compare the language concepts

| Language construct | Meaning in this lesson | Important limit |
| --- | --- | --- |
| Umicom C `const int` | A local is initialized once and source updates are refused | Initializer may execute at runtime; no const wire metadata |
| FreeBASIC `Const` | A named constant expression in the selected dialect | Follow FreeBASIC type and constant-expression rules |
| D `const` | A transitive read-only view of data | Another permitted mutable alias may still change the data |
| D `immutable` | Data that cannot be changed through any legal alias after construction | Conversion and ownership rules are stricter than a read-only view |

These are related ideas with different contracts. Do not translate keywords
mechanically without checking initialization, aliasing and lifetime rules.

## Practice and verification

1. Capture a mutable local, change the original, and predict the captured result.
2. Put a forbidden write in an unselected `else` branch and observe `check`.
3. Compare `trace` with `trace-bytecode` after compiling to a new output path.
4. Explain why a successful syntax check does not prove division is safe.
5. Inspect `tests/test_readonly_locals.c`: the cases cover every update spelling,
   both qualifier orders, boundary spans, local capacity, runtime errors and
   source/wire parity. `tests/check_cli_readonly_locals.cmake` checks the real
   hosted command and output-file preservation.
