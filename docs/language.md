![Umicom](assets/branding/umicom-logo.svg)

# Umicom C integer expression language

<!-- The original arithmetic-only title is retained for language-profile history: # Umicom C arithmetic language -->

This implementation compiles one small, explicit C-shaped language. It is not a complete C23 compiler. The implementation is written in C23 and is built with an existing C compiler.

A complete accepted program looks like this:

```c
int main(void) {
    return 2 + 3 * 4;
}
```

The result is 14. Multiplication is evaluated before addition. Parentheses change the grouping: `(2 + 3) * 4` produces 20.

## Words and punctuation

`int` introduces an integer result, `main` names the only supported entry function, and `void` says that it takes no arguments. Curly braces enclose its body. `return` supplies the result. The semicolon ends that statement.

<!-- Earlier profile wording retained for review:
The compiler requires these words exactly. `main()`, additional functions, declarations, variables, loops, calls, includes, strings and floating-point values are not accepted.
-->
<!-- Earlier grammar retained for review:
The compiler requires these entry-function words exactly. It also accepts initialized `int` locals, assignments and braced conditional statements before the final return. `main()`, additional functions, standalone nested blocks, loops, calls, includes, strings and floating-point values are not accepted.
-->
The compiler requires these entry-function words exactly. It accepts initialized int locals, assignments, braced conditionals and standalone compound statements before the final return. main(), additional functions, loops, calls, includes, strings and floating-point values remain unsupported. A standalone block introduces a lexical scope and executes whenever reached.

<!-- Earlier program production retained for review: program = "int" "main" "(" "void" ")" "{" "return" expression ";" "}" end -->

<!-- Earlier logical-root production: expression = logical-and ("||" logical-and)* -->

<!-- Earlier equality-root production: expression = relational (("==" | "!=") relational)* -->

The grammar below is a compact way to describe the accepted shapes. A quoted item is literal source text. A vertical bar means "choose one". An asterisk means "repeat zero or more times". A trailing question mark outside quoted text means "optional".

<!-- The earlier assignment production accepted only identifier = expression; it remains supported by the expanded assignment-operator rule below. -->

<!-- Earlier production retained for language history: assignment-operator = "=" | "+=" | "-=" | "*=" | "/=" | "%=" -->

<!-- Earlier production retained for language history: logical-and = equality ("&&" equality)* -->

<!-- Earlier update production retained for grammar review: update     = identifier assignment-operator expression ";" -->

<!-- Prior declaration spelling retained for grammar history: statement  = "int" identifier "=" expression ";" | update | conditional-statement -->

<!-- Prior body production retained for grammar history: body = "{" (update | conditional-statement)* "}" -->

<!-- Prior statement production retained: statement = local-declaration | update | conditional-statement -->

<!-- Earlier grammar retained for review: relational used additive operands directly; assignment-operator omitted <<= and >>=. The current productions below include the already implemented checked-shift profile. -->

<!-- Previous program/statement productions retained for grammar review: program ended immediately after the return semicolon; statement was local-declaration | update | conditional-statement | body. Empty statements extend those lists without a runtime instruction. -->

```text
program    = "int" "main" "(" "void" ")" "{" statement* "return" expression ";" empty-statement* "}" end
statement  = local-declaration | update | conditional-statement | body | empty-statement
empty-statement = ";"
local-declaration = ("const" "int" | "int" "const"?) declarator ("," declarator)* ";"
declarator = identifier "=" expression
update     = identifier assignment-operator expression ";" | ("++" | "--") identifier ";" | identifier ("++" | "--") ";"
conditional-statement = "if" "(" expression ")" body ("else" (body | conditional-statement))?
body       = "{" statement* "}"
assignment-operator = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "^=" | "|=" | "<<=" | ">>="
expression = logical-or ("?" expression ":" expression)?
logical-or = logical-and ("||" logical-and)*
logical-and = bitwise-or ("&&" bitwise-or)*
bitwise-or = bitwise-xor ("|" bitwise-xor)*
bitwise-xor = bitwise-and ("^" bitwise-and)*
bitwise-and = equality ("&" equality)*
equality   = relational (("==" | "!=") relational)*
relational = shift (("<" | "<=" | ">" | ">=") shift)*
shift      = additive (("<<" | ">>") additive)*
additive   = product (("+" | "-") product)*
product    = unary (("*" | "/" | "%") unary)*
unary      = ("+" | "-" | "!" | "~") unary | integer | ascii-character | boolean-literal | identifier | "(" expression ")"
boolean-literal = "true" | "false"
ascii-character = "'" (printable-ascii | simple-escape | octal-escape | hexadecimal-escape) "'"
integer    = decimal | ("0x" | "0X") hexadecimal | ("0b" | "0B") binary
decimal    = decimal-digit ("'"? decimal-digit)*
hexadecimal = hex-digit ("'"? hex-digit)*
binary     = binary-digit ("'"? binary-digit)*
```

<!-- The prior unary grammar is retained for review: unary = ("+" | "-") unary | integer | "(" expression ")". The active grammar adds ! at this same precedence level. -->

Repeated subtraction and division group from the left: `20 / 5 / 2` means `(20 / 5) / 2`, giving 2.

<!-- The earlier arithmetic-only grammar used: expression = product (("+" | "-") product)*
It is retained here because the additive production preserves that behaviour. -->

## Integer comparisons

Use `<`, `<=`, `>`, `>=`, `==` and `!=` to compare two integer values. Each produces an integer: 1 for true and 0 for false. Arithmetic binds more tightly than relational comparisons, and relational comparisons bind more tightly than equality. Thus `2 + 3 < 2 * 3` returns 1, while `2 == 3 < 4` means `2 == (3 < 4)` and returns 0.

Operators at the same level group from the left. `1 < 2 < 1` means `(1 < 2) < 1` and returns 0; it does not express a mathematical interval. Use parentheses to make your intent clear. The parser consumes `<=` as one token, but `< =` is invalid. <!-- Earlier profile wording retained for review:
Assignment, `&&`, `||`, conditionals and branches remain unsupported.
-->
<!-- Earlier profile wording retained for review:
Assignment is a statement, not an expression. `&&`, `||`, conditionals and branches remain unsupported.
-->
<!-- Earlier profile wording retained for review:
Assignment is a statement, not an expression. `&&` and `||` have short-circuit semantics below equality precedence. Conditional statements, conditional expressions and loops remain unsupported.
-->
Assignment is a statement, not an expression. `&&` and `||` have short-circuit semantics below equality precedence. The integer conditional expression `condition ? true_value : false_value` binds below logical OR. Braced conditional statements are supported as described below. Loops remain unsupported.

<!-- Earlier grammar restriction retained for review: Assignment, unary `!`, `&&`, `||`, conditionals and branches remain unsupported. -->

Both operands execute before the comparison. `0 == (1 / 0)` fails at division; equality does not suppress a failing operand. Comparison itself does not subtract the operands, so comparing the minimum and maximum signed values does not overflow. One comparison consumes one instruction of the execution budget, like one addition.

<!-- Before checked remainder, assignment-operator ended at "/=" and product accepted only "*" or "/". Those productions remain subsets of the current grammar. -->

## Numbers and failure

Values use signed 32-bit integers, from -2147483648 to 2147483647. Every integer token, whatever its spelling, may contain only a value between zero and 2147483647. The minus sign is a separate operation, so write the minimum value as `(-2147483647 - 1)`.

Leading zeros remain decimal in this language. For example, `010` is ten; this differs from traditional C octal spelling. Explicit `0x` / `0X` hexadecimal and `0b` / `0B` binary prefixes are supported. <!-- Earlier restriction retained for grammar history: Octal prefixes, suffixes, unsigned types and digit separators remain unsupported. -->
Octal prefixes, suffixes and unsigned types remain unsupported. One apostrophe may separate adjacent digits within a decimal, hexadecimal or binary number. A separator cannot follow a prefix, precede the end of a token or touch another separator.

<!-- Earlier profile restriction retained for review: Hexadecimal, octal prefixes, suffixes and digit separators are unsupported. The new lexer extends explicit prefixes while preserving the earlier decimal branch. -->

A prefix needs at least one valid digit. `0x2a`, `0X2A`, `0b101010` and decimal `42` all mean forty-two. Hexadecimal digits accept both cases of A through F. Whitespace and comments cannot split a prefix from its digits. `0b102`, `0x1u`, `0x1.0` and `0x1p0` are rejected. `0x80000000` is out of range; a preceding minus does not widen the token. Use `(-0x7fffffff - 1)` for the minimum signed value.

Only spelling changes. Each constant emits the existing PUSH operation; hexadecimal or binary text does not add an execution step or select a new wire format. Source offsets still point to the first byte of the token. See [numeric literal walkthrough](integer-literals.md) for commands, failures and compatibility checks.

Division truncates toward zero. For example, `-7 / 3` gives -2. Division by zero, negating the minimum value, and results outside the signed range are errors. They do not wrap.

The compiler checks syntax and instruction structure. Runtime arithmetic errors are detected when the program executes. Therefore `check` can accept `return 1 / 0;`, while `run` rejects it.

## Text and limits

Spaces, tabs, line endings, `// line comments` and `/* block comments */` separate tokens. Block comments do not nest. Embedded zero bytes are rejected, including inside comments. Source keywords and punctuation use ASCII; the implementation does not provide Unicode identifiers or byte-order-mark handling.

The source limit is 65,536 bytes. A compiled program has at most 256 instructions, its operand stack has at most 64 values, and expression recursion is bounded. Limits produce diagnostics instead of allocating unbounded storage.

Diagnostics use one-based line and byte-column positions. A CRLF line ending counts as one line break. Columns count bytes, not displayed character widths.

## Read the implementation

- [lexer.c](../src/lexer.c) recognises words, punctuation, comments and numbers.
- [compiler.c](../src/compiler.c) checks the grammar and emits operations.
- [vm.c](../src/vm.c) validates and executes the operations.
- [test_roundtrip_corpus.c](../tests/test_roundtrip_corpus.c) compares a fixed corpus with ordinary C arithmetic.

These limits describe the current implementation. They do not establish memory safety for unrelated native C code or compatibility with arbitrary C source.

## Logical negation

The prefix operator `!` returns 1 when its operand is zero, and 0 otherwise. It binds at the same level as unary minus: `!0 + 4` returns 5, while `!(0 + 4)` returns 0. Repeated negation groups from the right; `!!-9` returns 1. The lexer keeps `!=` as a single comparison token. Writing `1 ! 0` is invalid.

The compiler evaluates the operand, pushes zero and emits equality. Those two extra instructions use the existing comparison bytecode profile, so no opcode or public structure layout changes. Their source locations identify the `!` byte. Execution budgets count both instructions, and inspection reports the additional constant and stack slot. `!0` needs four instructions including its operand and return.

Negation does not suppress errors: `!(1 / 0)` fails while evaluating division. Existing source, recursion, instruction and stack limits continue to apply. The `UMC_LOGICAL_NOT_PROFILE` header macro identifies this source-grammar capability; separately installed libraries still need identity checks.

## Initialized local integers

A local names an integer stored during this execution. Declare it with `int count = 6;`, assign a new value with `count = count * 7;`, and read it with `return count;`. The initializer may read earlier declarations. The new name becomes visible only after its initializer has compiled. Self-reference, forward reference, duplicate declarations and reads of undeclared names are errors.

Names contain ASCII letters, digits and underscore, begin with a letter or underscore, are case sensitive and have at most 63 bytes. The keywords `int`, `main`, `void`, `return`, `if` and `else` are reserved. <!-- Earlier capacity wording: There are at most 32 locals per program. --> There are at most 32 simultaneously visible locals; disjoint bodies can reuse slots. Their storage belongs to one execution; it does not survive a later call. Every declaration requires an initializer. <!-- Previous restriction retained for review: There is one flat scope and no comma declaration, address operator, pointer, array or increment operation. -->
<!-- Earlier scope restriction retained for engineering history:
There is one flat scope and no comma declaration, address operator, pointer or array. Standalone increment and decrement are described below.
-->
<!-- Earlier single-declarator scope wording retained for review: Conditional bodies introduce lexical local scopes. A name disappears from lookup at the end of its body; separate bodies may reuse it. Declaring a name that is already visible is refused. There is no comma declaration, address operator, pointer or array. Standalone increment and decrement are described below. -->
Conditional bodies introduce lexical local scopes. A name disappears from lookup at the end of its body; separate bodies may reuse it. Declaring a name that is already visible is refused. A declaration can list multiple initialized names separated by commas; each initializer can read names earlier in that list. Address operators, pointers and arrays remain unsupported. Standalone increment and decrement are described below.

Read [the local integer walkthrough](local-integers.md) for lowering, slot validation, tracing and recovery.

## Short-circuit logical expressions

`a && b` evaluates `a` first and skips `b` when `a` is zero. `a || b` skips `b` when `a` is nonzero. Both return exactly 0 or 1. Equality binds more tightly than `&&`, and `&&` more tightly than `||`. Use `divisor != 0 && 42 / divisor > 1` to guard division. Reversing the operands removes that protection.

The compiler emits forward-only branch instructions and selects canonical format 4. The whole program is structurally validated, including the skipped arm, but skipped arithmetic is not evaluated. <!-- Earlier restriction retained: Single `&` and `|` remain unsupported. -->
Single `&` and `|` are eager bitwise operators, described below. Read [short-circuit evaluation](short-circuit.md) for trace and fuel examples.

## Conditional values

`condition ? when_true : when_false` evaluates the condition once, then evaluates only one arm. Nonzero selects the first arm; zero selects the second. The selected integer is preserved, so `1 ? -9 : 42` returns -9 rather than 1. Both arms must still parse and pass structural admission. Nesting associates to the right: `a ? b : c ? d : e` means `a ? b : (c ? d : e)`.

Every recursive arm consumes the same parse-depth allowance used by parentheses and unary operators. The compiler uses existing JUMP_IF_ZERO and JUMP operations; format 4 and the runtime feature mask do not change. No comma expressions, assignment expressions, omitted middle operand, pointer conversions or if statements are added by this expression extension; the separate statement extension is described below. See [conditional values](conditional-values.md) for commands, limits and recovery.

## Compound assignment statements

An initialized local can be updated with `value += expression;`, `-=`, `*=` or `/=`. The two operator characters must be adjacent. The compiler loads the old value once, evaluates the right expression once, applies existing checked arithmetic and stores the result. The update is a statement only: `int x += 1;`, `return x += 1;` and chained assignments are rejected. Arithmetic errors preserve the caller's completed-result storage; no partial assignment result is exposed. This is compiler capability bit 2048 and adds no runtime opcode or wire version. See [compound assignments](compound-assignments.md) for the source, trace and bytecode walkthrough.

## Conditional update statements

Use **if (expression) { updates }** with an optional **else { updates }**. Zero chooses the false path; any nonzero integer chooses the true path. <!-- Earlier scope restriction retained for engineering history:
Each body contains assignments or nested braced if statements, with no new declarations or early returns.
-->
<!-- Earlier grammar retained for review:
Each body contains initialized local declarations, assignments, increments or nested braced if statements. Early returns remain unsupported. The condition executes once. Both arms must compile, even when one is skipped. Braces are mandatory around update bodies. Else-if chains are accepted as successive false-path conditions; standalone scopes remain outside the grammar. Each additional if consumes the shared parse-depth allowance.
-->
Each body contains initialized local declarations, assignments, increments, nested braced if statements or standalone compound statements. Early returns remain unsupported. The condition executes once. Both arms must compile, even when one is skipped. Braces are mandatory around update bodies. Else-if chains are successive false-path conditions. Each additional if or standalone block consumes the shared parse-depth allowance; expressions share that same budget. See [standalone scopes](compound-statements.md) for the eight-instruction example and lifetime rules.

<!-- Earlier scope restriction retained for engineering history:
All body names must already be initialized in the flat outer scope.
-->
A body can read initialized outer locals and its own earlier declarations. A name cannot escape its body or be read by a sibling branch. Nested statements and their expressions share the 64-level parse bound. The existing forward-only runtime checks stack joins and local initialization; canonical wire format 4 is reused without new opcode identities. Source capability bit 4096 identifies the linked compiler support. Read [conditional updates](if-statements.md) for exact instruction paths and failure cases.

<!-- The prior conditional production required an else body: conditional-statement = "if" "(" expression ")" body ("else" body)?. It is retained here for grammar review; the active production also accepts a conditional statement after else. -->

Read [ordered else-if chains](else-if-chains.md) for selection, fuel and diagnostic examples.

## Checked signed remainder

The binary operator **%** binds alongside multiplication and division, grouping from the left. Thus 20 % 6 * 3 is (20 % 6) * 3, giving six. A nonzero remainder has the sign of the left operand: -7 % 3 is -1 and 7 % -3 is 1. Zero divisors and the minimum signed value paired with -1 fail before the host remainder operation. The compound statement **value %= expression;** updates one initialized local and evaluates its old value and right expression once. Assignment expressions remain unsupported.

Remainder uses opcode 18 and canonical format 5, even when branches appear earlier in the program. Programs without remainder retain their existing minimal formats and bytes. Read [checked remainder](remainder.md) for the exact fixture, capability checks, failure-preserving execution and consumer qualification.

<!-- Prior unary production retained for review: unary      = ("+" | "-" | "!") unary | integer | identifier | "(" expression ")" -->

## Signed bitwise complement

Unary **~** flips the signed 32-bit representation. ~0 is -1, ~-1 is zero, and ~(-2147483647-1) is 2147483647. It binds beside unary plus, minus and logical not. The compiler lowers it to PUSH -1, one evaluation of its operand and SUBTRACT, preserving endpoint behavior. Each complement adds two instructions. <!-- Earlier scope retained: Binary bitwise operators, shifts and ~= remain unsupported. -->
<!-- Earlier restriction retained for language-history review: Binary &, ^ and | are supported as described below. Shifts and ~= remain unsupported. -->
Binary &, ^ and | are supported as described below. Checked nonnegative shifts are supported by the separate shift extension; ~= remains unsupported. See [the complement walkthrough](complement.md) for exact fuel, source spans and unchanged wire profiles.

## Binary bitwise operators

Binary &, ^ and | operate eagerly on the signed 32-bit representation. Equality binds more tightly than &, then ^, then |; logical && and || bind more loosely. Initialized locals also accept &=, ^= and |= as statements. The source and runtime advertise UMC_FEATURE_BINARY_BITWISE; emitted bitwise operations require wire format 6. See [binary bitwise operators](bitwise.md) for grouping, signed endpoints, exact fuel and independent consumer checks.

## Standalone increment and decrement

The statements `++value;`, `value++;`, `--value;` and `value--;` update an initialized integer local by one. Both spellings discard the value of the update, so their behavior is identical in this limited grammar. Expression forms such as `return value++;` remain unsupported. The adjacent pair is one token; `+ +value` is not increment. Updates are also allowed inside existing braced conditional bodies. Each emits LOAD_LOCAL, PUSH 1, ADD or SUBTRACT, STORE_LOCAL, with checked signed arithmetic. Source capability bit 131072 advertises this syntax. No runtime feature bit or wire version is added. Read [step-by-step integer updates](increment-statements.md) for commands, fuel, overflow and verification.

<!-- Earlier integer productions retained for grammar history: integer = decimal | ("0x" | "0X") hex-digit+ | ("0b" | "0B") binary-digit+; decimal = decimal digit followed by zero or more decimal digits. -->

## Read-only local integers

<!-- Earlier scope restriction retained for engineering history:
An initialized top-level local may be declared with `const int` or `int const`.
-->
An initialized local in the top-level sequence or a conditional body may be declared with `const int` or `int const`.
The compiler refuses every later source update to that name, including writes
in unselected branches. The initializer may read earlier locals and still runs
<!-- Earlier scope restriction retained for engineering history:
at execution time. One qualifier is accepted; repeated qualifiers and declarations
inside conditional bodies remain outside this language. `const` is reserved,
-->
at execution time. One qualifier is accepted; repeated qualifiers remain outside
this language. Conditional-body declarations follow lexical scope. `const` is reserved,
while longer names such as `constant` remain identifiers.

The existing runtime slots and bytecode formats are unchanged. This is a source
contract, not immutable bytecode storage or full C23 type qualification. See
[the read-only-local walkthrough](readonly-locals.md) for commands, failures,
comparison with FreeBASIC and D, and the precise lifetime boundary.

## Checked nonnegative shifts

The compiler now supports `<<`, `>>`, `<<=` and `>>=` with a nonnegative left input, a count from 0 through 31, and a signed 32-bit representable result. Addition binds more tightly; comparisons bind less tightly. Invalid executed shifts report an arithmetic error rather than wrapping. See [checked shifts](checked-shifts.md) for source-check versus execution examples, precedence and failure ownership. Negative left inputs remain outside this checked subset.

## Empty statements

A standalone semicolon is accepted in existing statement lists and after the final return before the closing brace. It emits no instruction and consumes no VM fuel; source bytes and lexer work still count. The final return remains required and must be the last nonempty statement. Conditional bodies still require braces. Read [empty statements](empty-statements.md) for exact cost, capability queries and invalid semicolon placement.

## ASCII character constants

The previous unary production admitted numeric integers, names and parenthesized expressions. It now also accepts a single ASCII character constant, such as 'A' or '\n', as an integer value. Direct spelling is printable ASCII except the delimiter and backslash; escape spelling can represent values 0 through 127. There is no new char type or string storage. See [the exact character profile](ascii-characters.md) for escapes, range refusal and supported boundaries.

<!-- Previous declaration production retained for grammar history: local-declaration = ("const" "int" | "int" "const"?) identifier "=" expression ";" -->

## Initialized declaration lists

Write `int a = 6, b = a * 7;` to declare two integer locals in order. Each name needs its own equals sign and initializer. The qualifier in `const int a = 6, b = a * 7;` or `int const a = 6, b = a * 7;` applies to every name. A name becomes visible only after its own initializer is compiled, so a later initializer can read an earlier name but cannot read itself or a future name. Commas separate declarations here; comma expressions and comma-separated assignment statements are still refused. Read [initialized declaration lists](declarator-lists.md) for exact lowering, failure cases and commands.

## Source context in diagnostics

Source errors and unused-local warnings include a bounded escaped line and an aligned caret. Byte columns retain their existing meaning; tabs, control bytes and non-ASCII bytes are made visible. Read [source diagnostic excerpts](diagnostic-excerpts.md) for CRLF, EOF, clipping and the reusable compiler API.


## Explicit Boolean literal words

The earlier unary production admitted integer and ASCII character constants. It now also accepts exact lowercase true and false, lowering to integer 1 and 0 through the same operations. These complete words are reserved; longer identifiers keep their normal meaning. Typed bool declarations are not added. See [Boolean literals](boolean-literals.md) for execution, compatibility and the source-only feature query.

<!-- The immediately preceding unary production is retained for review:
unary      = ("+" | "-" | "!" | "~") unary | integer | ascii-character | identifier | "(" expression ")"
The active production accepts Boolean literal spellings through the existing integer lowering. -->
