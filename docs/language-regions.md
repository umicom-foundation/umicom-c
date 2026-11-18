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

<!-- Prior tag inventory retained: Accepted names are lowercase **umicom**, **c23**, **basic** and **java**. -->
Accepted names are lowercase **umicom**, **c23**, **basic**, **java** and **d**. A name requires trailing whitespace. Tags cannot nest. Unknown names, unmatched closings, missing closings, NUL bytes and unterminated quotes/comments are refused. Source is bounded by the compiler's source limit and 64 regions.

<!-- Earlier boundary description retained for review:
Delimiters in ordinary quoted strings and C-style comments are protected. BASIC regions also recognize doubled quotes and apostrophe comments. This is not a complete Java or BASIC lexer: Java Unicode escape preprocessing/text blocks and BASIC REM comments are outside the contract. Use separate files for those forms.
-->
<!-- Earlier REM-only boundary contract retained for review:
Delimiters in ordinary quoted strings and C-style comments are protected. BASIC regions recognize doubled quotes, apostrophe comments and the bounded REM form described below. This is not a complete Java or BASIC lexer: Java Unicode escape preprocessing/text blocks, BASIC line continuation and BASIC block comments remain outside the contract. Use separate files for those forms.
-->
Delimiters in ordinary quoted strings and C-style comments are protected. BASIC regions recognize doubled quotes, apostrophe comments, bounded REM comments and nested block comments as described below. This remains an experimental envelope scanner: Java Unicode escape preprocessing/text blocks, BASIC line continuation and metacommand interpretation are not implemented.

The library entry point is **umc_source_regions** in **umicom_c/source_regions.h**. Source storage is borrowed during the call; returned spans identify the original source. Inspect regions only on success. Failure returns a zero count; partially written array entries are not results.

## Current boundary

The **check**, **compile**, **run** and **debug** commands accept only the documented Umicom integer language. They do not execute tagged files. Naming a region C23 does not provide a full C23 compiler. <!-- Previous foreign-language boundary: BASIC/Java compilation, shared variables and cross-language calls are not implemented. -->
BASIC/Java/D compilation, shared variables and cross-language calls are not implemented.

A future mixed-language system needs separate parsers/type checking, a common intermediate representation, module boundaries, calling conventions, object ownership, string encodings, exception rules, library resolution and diagnostic maps. Java reflection, class loading and managed references cannot be preserved by replacing tags with C text.

Begin with independently compiled modules exchanging fixed-width scalar values through named functions. Preserve older bytecode readers by versioning new types/opcodes. Qualify each frontend and every supported boundary before advertising an executable envelope.

## BASIC statement comments

`REM` begins a comment at the start of a BASIC line or after a colon, allowing
leading spaces or tabs. Case does not matter. The three letters must be followed
by whitespace or the end of the supplied source span. The comment ends at a
carriage return or line feed. Delimiters inside it are ordinary comment text.

~~~text
<?basic
  REM These ?> and <?java characters do not close or nest a region.
  PRINT 42 : rEm This ?> is also comment text.
?>
~~~

The scanner returns one BASIC region for this exact envelope. The region still
contains the original comment bytes; it neither removes them nor rewrites the
source. A header comment or text outside this envelope adds an ordinary Umicom
region, so complete example files may have additional rows.

`Remember`, `rem_value` and `REM$` do not match the keyword. `PRINT REM` does
not place REM at a statement start. `REM?>` lacks the required boundary and
does not start a comment under this explicitly bounded scanner contract. These
examples describe classification, not whether the complete text is legal BASIC.

Run the new complete envelope with a built tool:

~~~powershell
.\build\bin\umicom-c.exe regions .\examples\basic_comments.umc
~~~

Use the location of your actual build directory. Expect BASIC and Umicom region
rows followed by the classification-only notice. Do not pass the envelope to
`run` expecting BASIC execution. Compile an independent `.bas` source with a
compatible FreeBASIC toolchain when learning executable BASIC. The BASIC course
explains both workflows and why a scanner is only one part of a compiler.


## Nested BASIC block comments

A block opens with slash-apostrophe and closes with apostrophe-slash. The scanner
counts nested openers, up to **UMC_BASIC_COMMENT_DEPTH_LIMIT (32)**. It uses an
integer counter, not recursive function calls or dynamically allocated storage.
The 65,536-byte source limit still applies. This is an Umicom envelope limit,
not a claim that FreeBASIC imposes the same nesting limit.

~~~text
<?basic
/' Outer comment with ?>
   /' Inner comment with <?java '/
   Still in the outer comment.
'/
DIM value AS /' a comment across
               two lines '/ INTEGER
?>
~~~

The example contains one BASIC region. Original comment bytes remain in its
span. While inside a block, quotation marks, apostrophes without a following
slash, colons and language tags are ordinary comment text. Nested block markers
still count even when they appear between quotation marks inside that comment.
Inside a real BASIC string, however, slash-apostrophe is only string content.
The existing single-line comment state ignores block markers until its newline.

Newlines inside a block do not end the enclosing statement. Consequently, a
block encountered at a statement start preserves that state for a following
REM. A block after PRINT does not make a following REM into a statement-start
comment, even if the block contains a newline or colon. This continues the
explicitly narrow REM contract above; it is not full FreeBASIC syntax admission.

An unclosed block reports the original outer opener. An attempted 33rd nesting
level reports that new opener. Both return UMC_SYNTAX_ERROR and a zero result
count. Discard all partial region-array entries on failure.

1. Open examples/basic_block_comments.umc and identify its outer and inner pairs.
2. Run the command below using your actual build directory.
3. Expect one BASIC row, ordinary Umicom rows for the source header/outer
   whitespace, and the classification-only notice.
4. In a separate copy, remove the final outer apostrophe-slash. Expect refusal
   rather than a successful region list. The original file should remain intact.
5. Do not run this envelope as BASIC. Use a separate .bas file and a FreeBASIC
   compiler to execute BASIC; the scanner does not supply that compiler.

~~~powershell
.\build\bin\umicom-c.exe regions .\examples\basic_block_comments.umc
~~~

For the source-language rules, see the
[FreeBASIC comment reference](https://www.freebasic.net/wiki/ProPgComments).

## D comments and raw strings

The lowercase D tag is followed by whitespace, just like the other tags.
Ordinary quoted strings and characters protect delimiters. The scanner also
protects r-prefixed double-quoted raw strings, backtick raw strings and nested
slash-plus comments. These states preserve original source bytes.

~~~text
<?d
/+ A comment containing ?>.
   /+ A nested comment containing <?java. +/
+/
string message = r"Display ?> as text.";
?>
~~~

1. Build the Umicom C tool using your platform preset.
2. Run **umicom-c regions examples/d_regions.umc** from the repository.
3. Expect one D row plus any Umicom rows for the leading source header and
   surrounding whitespace. Inspect the language and original byte spans.
4. Remove the outer plus-slash from a copy. Expect a syntax diagnostic and no
   published region rows.
5. Do not pass an envelope to **run** expecting D execution. Compile a separate
   .d file with an actual D compiler for that purpose.

Nested D comments have a separate **UMC_D_COMMENT_DEPTH_LIMIT (32)**. This is
an envelope resource bound, not a statement about D compiler capacity. Quotes
inside those comments do not suppress nested markers. Raw strings close only
on their matching quote or backtick; backslashes inside them are ordinary bytes.

D q-delimited/token strings, x-prefixed hexadecimal strings and i-prefixed
interpolation are explicitly refused at their prefix. They need a fuller D
lexical implementation; keep them in separate D files. Unicode source
normalization and D syntax/type checking are not provided. A successful
classification can therefore include text that is not a valid D program.

D is appended to the language enumeration; the existing Umicom, C23, BASIC and
Java numeric identities are unchanged. No runtime opcode, bytecode format,
compiler grammar or execution permission changes with this addition.

Read the [D lexical reference](https://dlang.org/spec/lex.html) for the complete
language rules and the Bits2Banking D lesson on comment/string boundaries for
the standalone D example.
