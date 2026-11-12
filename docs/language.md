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
The compiler requires these entry-function words exactly. It also accepts initialized `int` locals and assignments before the final return. `main()`, additional functions, nested blocks, loops, calls, includes, strings and floating-point values are not accepted.

<!-- Earlier program production retained for review: program = "int" "main" "(" "void" ")" "{" "return" expression ";" "}" end -->

<!-- Earlier logical-root production: expression = logical-and ("||" logical-and)* -->

<!-- Earlier equality-root production: expression = relational (("==" | "!=") relational)* -->

The grammar below is a compact way to describe the accepted shapes. A quoted item is literal source text. A vertical bar means "choose one". An asterisk means "repeat zero or more times". A trailing question mark outside quoted text means "optional".

```text
program    = "int" "main" "(" "void" ")" "{" statement* "return" expression ";" "}" end
statement  = "int" identifier "=" expression ";" | identifier "=" expression ";"
expression = logical-or ("?" expression ":" expression)?
logical-or = logical-and ("||" logical-and)*
logical-and = equality ("&&" equality)*
equality   = relational (("==" | "!=") relational)*
relational = additive (("<" | "<=" | ">" | ">=") additive)*
additive   = product (("+" | "-") product)*
product    = unary (("*" | "/") unary)*
unary      = ("+" | "-" | "!") unary | integer | identifier | "(" expression ")"
integer    = decimal | ("0x" | "0X") hex-digit+ | ("0b" | "0B") binary-digit+
decimal    = decimal digit followed by zero or more decimal digits
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
Assignment is a statement, not an expression. `&&` and `||` have short-circuit semantics below equality precedence. The integer conditional expression `condition ? true_value : false_value` binds below logical OR. Conditional statements and loops remain unsupported.

<!-- Earlier grammar restriction retained for review: Assignment, unary `!`, `&&`, `||`, conditionals and branches remain unsupported. -->

Both operands execute before the comparison. `0 == (1 / 0)` fails at division; equality does not suppress a failing operand. Comparison itself does not subtract the operands, so comparing the minimum and maximum signed values does not overflow. One comparison consumes one instruction of the execution budget, like one addition.

## Numbers and failure

Values use signed 32-bit integers, from -2147483648 to 2147483647. Every integer token, whatever its spelling, may contain only a value between zero and 2147483647. The minus sign is a separate operation, so write the minimum value as `(-2147483647 - 1)`.

Leading zeros remain decimal in this language. For example, `010` is ten; this differs from traditional C octal spelling. Explicit `0x` / `0X` hexadecimal and `0b` / `0B` binary prefixes are supported. Octal prefixes, suffixes, unsigned types and digit separators remain unsupported.

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

Names contain ASCII letters, digits and underscore, begin with a letter or underscore, are case sensitive and have at most 63 bytes. The keywords `int`, `main`, `void` and `return` remain reserved. There are at most 32 locals per program. Their storage belongs to one execution; it does not survive a later call. Every declaration requires an initializer. There is one flat scope and no comma declaration, address operator, pointer, array or increment operation.

Read [the local integer walkthrough](local-integers.md) for lowering, slot validation, tracing and recovery.

## Short-circuit logical expressions

`a && b` evaluates `a` first and skips `b` when `a` is zero. `a || b` skips `b` when `a` is nonzero. Both return exactly 0 or 1. Equality binds more tightly than `&&`, and `&&` more tightly than `||`. Use `divisor != 0 && 42 / divisor > 1` to guard division. Reversing the operands removes that protection.

The compiler emits forward-only branch instructions and selects canonical format 4. The whole program is structurally validated, including the skipped arm, but skipped arithmetic is not evaluated. Single `&` and `|` remain unsupported. Read [short-circuit evaluation](short-circuit.md) for trace and fuel examples.

## Conditional values

`condition ? when_true : when_false` evaluates the condition once, then evaluates only one arm. Nonzero selects the first arm; zero selects the second. The selected integer is preserved, so `1 ? -9 : 42` returns -9 rather than 1. Both arms must still parse and pass structural admission. Nesting associates to the right: `a ? b : c ? d : e` means `a ? b : (c ? d : e)`.

Every recursive arm consumes the same parse-depth allowance used by parentheses and unary operators. The compiler uses existing JUMP_IF_ZERO and JUMP operations; format 4 and the runtime feature mask do not change. No comma expressions, assignment expressions, omitted middle operand, pointer conversions or if statements are added. See [conditional values](conditional-values.md) for commands, limits and recovery.
