"" Vim syntax file
" Language: YLC
" Maintainer: AI Assistant
" Latest Revision: 2024-10-20

" if exists("b:current_syntax")
"   finish
" endif

" Keywords
syn keyword ylcKeyword fn let in and extern true false match with import type of if include contained
syn match ylcKeywordMatch "\<\(fn\|let\|in\|and\|extern\|true\|false\|match\|with\|import\|type\|of\|if\|include\)\>" contains=ylcKeyword

" Types
syn keyword ylcType void contained
syn match ylcTypeMatch "\<void\>" contains=ylcType

" Operators
syn match ylcOperator "\v[-+*/=<>!&|%^]"
syn match ylcOperator "\v(\>\=|\<\=|\=\=|\!\=)"
syn match ylcOperator "\v(\:\:|-\>|\|>|\@\@)"

" Identifiers
syn match ylcIdentifier "\v[a-zA-Z_][a-zA-Z0-9_]*" contains=ylcKeywordMatch,ylcTypeMatch
syn match ylcMetaIdentifier "\v\@[a-zA-Z_][a-zA-Z0-9_]*"

" Numbers
syn match ylcNumber "\v<-?\d+>"
syn match ylcFloat "\v<-?\d+f>"
syn match ylcFloat "\v<-?\d*\.\d+>"
syn match ylcHex "\v<0x[0-9a-fA-F]+>"

" Strings
syn region ylcString start=/"/ skip=/\\"/ end=/"/ contains=ylcEscape
syn region ylcFString start=/`/ end=/`/ contains=ylcFStringInterp
syn region ylcTripleFString start=/```/ end=/```/ contains=ylcFStringInterp
syn region ylcFStringInterp start=/{/ end=/}/ contained

" Escape sequences
syn match ylcEscape contained "\\['"\\abfnrtv]"
syn match ylcEscape contained "\\\d\{1,3}"
syn match ylcEscape contained "\\x[0-9a-fA-F]\{2}"

" Characters
syn match ylcChar "'[^']'"
syn match ylcChar "'\\['"?\\abfnrtv]'"
syn match ylcChar "'\\[0-7]\{1,3}'"
syn match ylcChar "'\\x[0-9a-fA-F]\+'"

" Comments
syn match ylcComment "#.*$"

" Define the default highlighting
hi def link ylcKeyword Keyword
hi def link ylcType Type
hi def link ylcOperator Operator
hi def link ylcIdentifier Function
hi def link ylcMetaIdentifier PreProc
hi def link ylcNumber Number
hi def link ylcFloat Float
hi def link ylcHex Number
hi def link ylcString String
hi def link ylcFString String
hi def link ylcTripleFString String
hi def link ylcFStringInterp Special
hi def link ylcChar Character
hi def link ylcComment Comment
hi def link ylcEscape Special

" let b:current_syntax = "ylc"
