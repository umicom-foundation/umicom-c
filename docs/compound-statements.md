![Umicom](assets/branding/umicom-logo.svg)

# Give a temporary calculation its own block

A compound statement is a group of statements enclosed by { and }. In Umicom C,
you can put such a group directly inside main or inside another supported body.
The opening brace begins a lexical scope: names declared inside it are usable
only up to its matching closing brace. This lets a calculation have temporary
names without pretending that a condition controls whether it runs.

## Start with a complete program

1. Open the Umicom C checkout. Use a configured hosted build whose executable
   path you know. The commands below assume build/bin; change that path if your
   build directory is different.
2. Open examples/compound_statements.uc. The leading comment identifies the
   product and licence. The program below that comment is executable source.
3. Run each command separately and stop on errors:

~~~powershell
.\build\bin\umicom-c.exe check .\examples\compound_statements.uc
.\build\bin\umicom-c.exe run .\examples\compound_statements.uc --max-steps 8
.\build\bin\umicom-c.exe trace .\examples\compound_statements.uc
~~~

Check should report eight accepted instructions. Run should print Result: 42
and Instructions: 8. The source is:

~~~c
int main(void) {
    int result = 0;
    {
        const int answer = 42;
        result = answer;
    }
    return result;
}
~~~

The inner block executes whenever control reaches it. There is no test to
select or skip it. result is declared in the outer body and remains visible
inside and after the block. answer is declared inside the block and disappears
from name lookup at its closing brace. The assignment copies the integer 42
into result while answer is still visible. The final return therefore does not
need to reach into a finished scope.

## Read what each brace means

The first opening brace belongs to main and starts its entry-function body.
The second begins the temporary scope. The first closing brace ends only that
temporary scope. The final closing brace ends main. Indentation helps a person
match those pairs; it does not replace punctuation or change the grammar.

An empty block, {}, is accepted and emits no instruction. A trailing semicolon
after a block is unnecessary and is rejected by this limited grammar: an empty
statement written as ; is not currently supported. The final return still
belongs to main. A return inside a nested block is an early return and remains
unsupported.

## Make a scope error and recover

1. Copy the example to a separate practice file.
2. Change return result to return answer and run check on that copy.
3. Expect a source diagnostic saying that the local must be declared and
   initialized before use. The name answer is outside its scope here.
4. Restore return result. Do not solve this error by increasing a memory limit;
   it is a source-name rule, not a shortage of storage.
5. Add int answer = 7; inside the same block. Expect a duplicate-name refusal.
6. Remove the added declaration. Try declaring another answer in a later,
   separate block. That is allowed because the earlier answer is no longer
   visible. Use this only when the repeated name makes the code clearer.

An active outer name cannot be redeclared in an inner block. Full C supports
many forms of shadowing; this bounded Umicom C profile deliberately refuses it.
Uninitialized locals, arrays, pointers and function calls also remain outside
the supported grammar.

## Distinguish a source name from a VM slot

The compiler keeps a bounded table of visible names. Each record identifies a
source spelling, a numeric local slot and whether source updates are allowed.
On entering a body it remembers the table length. It compiles statements in
order, then restores that length on exit, including an error exit. An initializer
is checked before its new name enters the table, so int value = value; cannot
read an uninitialized value through its own declaration.

The virtual machine, or VM, executes numeric instructions. It does not keep
these source names. Entering and leaving a block emits no instruction. A later
scope may reuse the same numeric slot, but it must initialize it before a read.
Scope exit does not erase memory, collect a heap object or release an operating
system resource. Do not use it as a security wipe or a resource-management API.

For the example, the eight operations are:

| Operation | Meaning |
| --- | --- |
| PUSH 0 | Prepare the outer initializer |
| STORE_LOCAL 0 | Initialize result |
| PUSH 42 | Prepare the temporary initializer |
| STORE_LOCAL 1 | Initialize answer |
| LOAD_LOCAL 1 | Read answer while visible |
| STORE_LOCAL 0 | Copy its value to result |
| LOAD_LOCAL 0 | Read the returned value |
| RETURN | Publish the completed result |

The const qualifier prevents source updates to answer. It does not add a new
immutable VM storage class. A source compilation check and runtime validation
check serve different purposes.

## Combine blocks and conditions

~~~c
int main(void) {
    int result = 0;
    {
        const int factor = 7;
        if (factor > 0) {
            { int amount = 6; result = amount * factor; }
        }
    }
    return result;
}
~~~

The standalone blocks above always execute when reached; the if body depends
on its condition. All source bodies must compile, including an unselected one.
A division by zero in an unselected initializer does not execute. An unknown
name or forbidden write in that initializer is still rejected during compilation.
Use trace to see the actual selected instructions rather than counting source
lines as execution steps.

There are at most 32 simultaneously visible locals. Three sibling blocks can
use 30 temporary names each while one outer result remains visible: their
lifetimes do not overlap. The complete program still has at most 256 instructions.
Blocks, conditions and expressions share the 64-level recursive parse budget.
A chain of empty standalone blocks isolates that structural limit; adding an
expression near the deepest point may consume more levels and be refused sooner.

## Save and execute the same instructions

Choose an output filename that does not already exist:

~~~powershell
.\build\bin\umicom-c.exe compile .\examples\compound_statements.uc .\scope-practice.umcb
.\build\bin\umicom-c.exe exec .\scope-practice.umcb --max-steps 8
.\build\bin\umicom-c.exe trace-bytecode .\scope-practice.umcb
.\build\bin\umicom-c.exe profile
~~~

This example uses existing canonical wire format 3 because it needs local slots
but no branch operation. Other operations can require a newer wire format.
Braces themselves select no format and change no opcode. A seven-step fuel
budget refuses execution before a completed result can be published. A failed
compile leaves the public instruction count at zero; its output array is scratch
storage and must not be executed.

The compiler reports UMC_FEATURE_COMPOUND_STATEMENTS, bit 2097152. The runtime
does not report this source-only feature. Query the linked compiler instead of
assuming that installing a new header upgrades an older library or executable.

## Verify a change to this feature

The regression source tests/test_compound_statements.c compares small values
with independently written native C, verifies exact eight-operation lowering,
wire roundtrips and fuel limits, and checks scope escape, shadowing, initialization,
read-only writes, malformed bodies, local capacity, slot reuse, shared depth,
bounded source spans and runtime arithmetic faults. The CLI check verifies real
commands and preservation of existing source and artifact files.

~~~powershell
ctest --test-dir build -R "umicom.c.(compound-statements|cli-compound-statements|branch-locals|cli-profile)" --output-on-failure --no-tests=error
~~~

These are checks you can run on the selected build. They do not qualify an
independent Framework installation, Studio process, OS image or full C23 compiler.
If check rejects a brace, read the profile and executable path first. If a name
is refused, inspect its nearest opening and closing braces and its declaration
order. Read [conditional scopes](branch-local-scope.md), [the grammar](language.md)
and [read-only locals](readonly-locals.md) when extending the example.
