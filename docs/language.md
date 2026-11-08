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

The compiler requires these words exactly. `main()`, additional functions, declarations, variables, loops, calls, includes, strings and floating-point values are not accepted.

The grammar below is a compact way to describe the accepted shapes. A quoted item is literal source text. A vertical bar means "choose one". An asterisk means "repeat zero or more times".

```text
program    = "int" "main" "(" "void" ")" "{" "return" expression ";" "}" end
expression = relational (("==" | "!=") relational)*
relational = additive (("<" | "<=" | ">" | ">=") additive)*
additive   = product (("+" | "-") product)*
product    = unary (("*" | "/") unary)*
unary      = ("+" | "-") unary | integer | "(" expression ")"
integer    = decimal digit followed by zero or more decimal digits
```

Repeated subtraction and division group from the left: `20 / 5 / 2` means `(20 / 5) / 2`, giving 2.

<!-- The earlier arithmetic-only grammar used: expression = product (("+" | "-") product)*
It is retained here because the additive production preserves that behaviour. -->

## Integer comparisons

Use `<`, `<=`, `>`, `>=`, `==` and `!=` to compare two integer values. Each produces an integer: 1 for true and 0 for false. Arithmetic binds more tightly than relational comparisons, and relational comparisons bind more tightly than equality. Thus `2 + 3 < 2 * 3` returns 1, while `2 == 3 < 4` means `2 == (3 < 4)` and returns 0.

Operators at the same level group from the left. `1 < 2 < 1` means `(1 < 2) < 1` and returns 0; it does not express a mathematical interval. Use parentheses to make your intent clear. The parser consumes `<=` as one token, but `< =` is invalid. Assignment, unary `!`, `&&`, `||`, conditionals and branches remain unsupported.

Both operands execute before the comparison. `0 == (1 / 0)` fails at division; equality does not suppress a failing operand. Comparison itself does not subtract the operands, so comparing the minimum and maximum signed values does not overflow. One comparison consumes one instruction of the execution budget, like one addition.

## Numbers and failure

Values use signed 32-bit integers, from -2147483648 to 2147483647. A decimal token may contain only a value between zero and 2147483647. The minus sign is a separate operation, so write the minimum value as `(-2147483647 - 1)`.

Leading zeros remain decimal in this language. For example, `010` is ten; this differs from traditional C octal spelling. Hexadecimal, octal prefixes, suffixes and digit separators are unsupported.

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
