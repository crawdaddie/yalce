" Quit when a syntax file was already loaded.
if exists('b:current_syntax') | finish|  endif

syntax match simpleVar "[a-zA-Z_][a-zA-Z_0-9]*" nextgroup=simpleAssignment
syntax match simpleAssignment "=" contained nextgroup=simpleValue
syntax match simpleString "\".*\""

hi def link simpleVar Identifier
hi def link simpleAssignment Statement
hi def link simpleString String

let b:current_syntax = 'simple'
