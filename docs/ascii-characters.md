![Umicom](assets/branding/umicom-logo.svg)

# Read ASCII character constants as integer values

Umicom C accepts a single ASCII character constant wherever an integer primary expression is accepted. The quoted spelling becomes an ordinary integer PUSH. It adds neither a char storage type nor strings, allocation or a new bytecode format. This remains a bounded C-shaped language, not a complete C23 implementation.

~~~c
int main(void) {
    int c = 'A';
    if (c >= 'A' && c <= 'Z') {
        c += 'a' - 'A';
    }
    return c;
}
~~~

The result is 97, the ASCII value of lowercase a. This demonstration handles ASCII uppercase letters only. It is not Unicode case conversion.

## Exact accepted spellings

A direct character is one printable ASCII byte from space (32) through tilde (126), except quote and backslash, which require escapes. Escape spelling can represent any value from 0 through 127. Escapes are decoded independently of the host C compiler’s char signedness.

| Spelling | Value or rule |
| --- | --- |
| '\a', '\b', '\f' | 7, 8, 12 |
| '\n', '\r', '\t', '\v' | 10, 13, 9, 11 |
| '\\', '\'', '\"', '\?' | 92, 39, 34, 63 |
| '\0' through octal '\177' | One to three octal digits, decoded value at most 127 |
| '\x0' through hexadecimal '\x7F' | One or more hex digits, decoded value at most 127; leading zeros allowed |

The hexadecimal prefix is lowercase x. Hex digits may use either case. The lexer consumes the whole hexadecimal digit run before requiring the closing quote; it does not stop after two digits. Numeric accumulation checks range before multiplication. Octal escapes consume at most three digits and then require the closing quote. Source-byte limits still bound long leading-zero spellings.

Raw control characters, raw non-ASCII bytes, empty or multi-character constants, wide/Unicode prefixes, Unicode escapes and unknown escapes are refused. Examples include empty quotes, 'ab', '\q', '\u0041', L'A' and u8'A'. Values such as '\200' and '\x80' return the literal-range status. '\0000' is a malformed multi-character spelling, not a different way to write zero.

A digit separator inside a number is still a separate lexical rule: 1'000 and 0x2'A remain numeric tokens. An opening quote at the start of a token begins a character constant. Adjacent constants without an operator do not concatenate. Comment markers inside valid character constants remain values, so '/' and '*' do not open comments.

## Run the supplied example

Use the Windows preset after selecting the UCRT64 toolchain. Run separately and stop on unexpected errors.

~~~powershell
Set-Location "C:\umicom\umicom-c"
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
cmake --preset windows-ucrt64-debug
cmake --build --preset windows-ucrt64-debug --parallel 2
.\build\windows-ucrt64-debug\bin\umicom-c.exe check examples/ascii_characters.uc
.\build\windows-ucrt64-debug\bin\umicom-c.exe run examples/ascii_characters.uc --max-steps 2
~~~

The example returns '*', ASCII value 42. Check reports two instructions: PUSH 42 and RETURN. Run prints Result: 42 and Instructions: 2. A fuel budget of one intentionally fails before publishing the result. Compile/exec use the existing format-1 representation; the PUSH source offset still identifies the opening quote.

## Use the matching compiler capability

UMC_ASCII_CHARACTER_PROFILE is one. Test UMC_FEATURE_ASCII_CHARACTER_LITERALS (33554432) in umc_compiler_profile(UMC_PROFILE_FEATURES). The runtime must not advertise it because it sees integer records, not character spelling. Existing runtime and wire feature masks are unchanged. A newer header does not retrofit this lexer into an older linked compiler.

The warning collector shares compiler name resolution. In int main(void){int A=1;return 'A';}, the local A is never read: a character constant containing that letter is not an identifier. Failed compilation publishes count zero, and partially written instruction storage must not be executed.

Run the lexical and CLI regressions with:

~~~powershell
ctest --test-dir build/windows-ucrt64-debug -R "umicom.c.(ascii-characters|cli-ascii-characters|profile|cli-profile)" --output-on-failure --no-tests=error
~~~

See [the grammar](language.md), [source warnings](source-warnings.md), and [integer spelling](integer-literals.md). This feature does not change source-region language selection or allow arbitrary embedded languages to execute.
