![Umicom](assets/branding/umicom-logo.svg)

# Discover tagged language regions

A language frontend reads source text, checks its grammar and types, and produces a representation for execution or code generation. A tag can select a frontend; it cannot replace one.

The **regions** command recognizes an experimental source envelope:

~~~text
<?umicom
int main(void) { return 42; }
?>
<?basic
DIM answer AS INTEGER = 42
?>
<?java
class Answer { int value = 42; }
?>
~~~

Run **umicom-c regions examples/language_regions.umc**. Output gives each region's language, byte offset and length, followed by a classification-only notice. Text outside tags belongs to the Umicom region. Whitespace between tags is included as an ordinary region; do not assume one row per opening tag.

Accepted names are lowercase **umicom**, **c23**, **basic** and **java**. A name requires trailing whitespace. Tags cannot nest. Unknown names, unmatched closings, missing closings, NUL bytes and unterminated quotes/comments are refused. Source is bounded by the compiler's source limit and 64 regions.

Delimiters in ordinary quoted strings and C-style comments are protected. BASIC regions also recognize doubled quotes and apostrophe comments. This is not a complete Java or BASIC lexer: Java Unicode escape preprocessing/text blocks and BASIC REM comments are outside the contract. Use separate files for those forms.

The library entry point is **umc_source_regions** in **umicom_c/source_regions.h**. Source storage is borrowed during the call; returned spans identify the original source. Inspect regions only on success. Failure returns a zero count; partially written array entries are not results.

## Current boundary

The **check**, **compile**, **run** and **debug** commands accept only the documented Umicom integer language. They do not execute tagged files. Naming a region C23 does not provide a full C23 compiler. BASIC/Java compilation, shared variables and cross-language calls are not implemented.

A future mixed-language system needs separate parsers/type checking, a common intermediate representation, module boundaries, calling conventions, object ownership, string encodings, exception rules, library resolution and diagnostic maps. Java reflection, class loading and managed references cannot be preserved by replacing tags with C text.

Begin with independently compiled modules exchanging fixed-width scalar values through named functions. Preserve older bytecode readers by versioning new types/opcodes. Qualify each frontend and every supported boundary before advertising an executable envelope.
