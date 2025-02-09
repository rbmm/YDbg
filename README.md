# undname
 
how is undecorate C++ symbol name ?
for this can be used or `UnDecorateSymbolName` api from Dbghelp.dll or `__unDNameEx` (have 3 additional parameters, compare to `UnDecorateSymbolName` ) from msvcrt.dll ot vcruntime*.dll
i not deep compare it implementation, but probably all it do the same. say util undname.exe from vs kit, use __unDNameEx from vcruntime140.dll

however all this implementation fail undecorate c++ string literals, by some reason. if pass such symbol (begin with `??_C@_` ) to it, they simply output `string'. 
sometime something like 
```
::FNODOBFM::`string'
```
so i decide do this by self. the c++ strings symbols have next form:

```
??_C@_ <0/1> <prefix> @ <string data> @ [suffix@]
```

all string begin with 6 symols sequense: `??_C@_`
the next symbols must be **0** - mean (char, ansi) or **1** - (wchar_t, unicode) string
**@** symbols in names used as delimiter

prefix can be 2 forms, depend on it first character.
are this is number ( '0'..'9') or not, in second case it containing one @ symbol internal

example of first form - `5HDPIMK`, here first char - '5' is number
example of seconf form - `O@MNIMFBBJ` - 'O' is not number and additional '@' exist

[suffix@] can be empty (not present) or by several letters (7-8 usual)

this is based on where string is located. by default string is put to `.rdata` section.
but if we use `/cbstring` compiler option, strings is put to `code$s` section, where is `code` section name of the code, which use string literal

if code in `.text` section, strings will be in `.text$s` (and use `FNODOBFM` suffix)
if code in `PAGE` section, strings will be in `PAGE$s` (and use `NNGAKEGL` suffix)
and so on. 
say ntoskrnl.exe containing near 7300+ strings with 8 different suffixes:

```
NNGAKEGL  000005ff PAGE
FNODOBFM  0000033d TEXT
GHGBBCHJ  0000032c
PBOPGDP   00000216 INIT
EKOMKFNL  000002fc
LBKOJDO   0000019e
JKADOLAD  000000d6 PAGEVRFY
CIFEBFPJ  00000037 PAGEHDLS
CIJCHKMG  00000031 PAGEBGFX
OKHAJAOM  0000002f PAGELK
DFIOBLLK  00000011 PAGEKD
NKKEPPGN  0000000b 
```
(the number in second column is count of strings with this suffix)

example of strings:
```
??_C@_1O@MNIMFBBJ@?$AA?$CI?$AAT?$AAR?$AAU?$AAE?$AA?$CJ@NNGAKEGL@  // L"(TRUE)" :: NNGAKEGL

1 - unicode 
`O@MNIMFBBJ` - prefix
?$AA?$CI?$AAT?$AAR?$AAU?$AAE?$AA?$CJ - string data
NNGAKEGL - suffix

??_C@_05HDPIMK@valid@FNODOBFM@ // "valid" :: FNODOBFM

0 - ansi
5HDPIMK - prefix
valid - string data
FNODOBFM - suffix
```
most interesting part is "string data". this is bytes (not symbols) array of string. maximum 31 bytes
so 31 symbols for ansi and 15 for unicode strings
in case unicode string, bytes go in big endian order - first is encoded high byte of wchar_t and then low byte
It is clear that some characters cannot be in a name - \0x0, \r, \n, '@', ' ', etc
for it is esc sequence is used.
esc begin with ? symbol
after it can be or $ symbol and then yet 2 characters
or one char otherwise, usual is number

say \0x0 encoded as "?$AA" (most frequently symbol for unicode strings)
"?0" is ',' , "?3" is ':' , '?$HN' is '[', "?$FN" is ']' and so on

To be honest, I couldn't understand this coding logic, I determined it experimentally

function for decode strings - https://github.com/rbmm/YDbg/blob/main/undname/ep.cpp#L103
(exist some more not printable esc sequence, 25 symbols from 7374 symbols from my ntoskrnl, is not decoded

list of ntos [strings](https://github.com/rbmm/YDbg/blob/main/undname/ntos-strings.txt)

[undname.exe](https://github.com/rbmm/YDbg/blob/main/x64/Release/undname.exe)

have next cmdline format: `*symbol[*symbol]*`

every symbol must begin and end with '*' (without spaces) utility can decode several symbols

say run `undname.exe *sym1*sym2*sym3*` show undecorated names for 3 symbols
