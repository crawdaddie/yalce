if !exists("main_syntax")
  " quit when a syntax file was already loaded
  if exists("b:current_syntax")
    finish
  endif
  let main_syntax = 'simple'
elseif exists("b:current_syntax") && b:current_syntax == "simple"
  finish
endif

let s:cpo_save = &cpo
set cpo&vim


syn keyword simpleCommentTodo      TODO FIXME XXX TBD contained
syn keyword simpleCommentExpect    expect contained
syn match   simpleLineComment      "#.*$" contains=@Spell,simpleCommentTodo,simpleCommentExpect
syn match   simpleCommentSkip      "^[ \t]*\*\($\|[ \t]\+\)"
syn region  simpleComment	   start="/\*"  end="\*/" contains=@Spell,simpleCommentTodo
syn match   simpleSpecial	   "\\\d\d\d\|\\."
syn region  simpleStringD	   start=+"+  skip=+\\\\\|\\"+  end=+"\|$+	contains=simpleSpecial,@htmlPreproc

" syn region  simpleEmbed	           start=+${+  end=+}+	contains=@simpleEmbededExpr

" number handling by Christopher Leonard chris.j.leonard@gmx.com
syn match   simpleNumber           "\<0[bB][0-1]\+\(_[0-1]\+\)*\>"
syn match   simpleNumber           "\<0[oO][0-7]\+\(_[0-7]\+\)*\>"
syn match   simpleNumber           "\<0\([0-7]\+\(_[0-7]\+\)*\)\?\>"
syn match   simpleNumber           "\<0[xX][0-9a-fA-F]\+\(_[0-9a-fA-F]\+\)*\>"
syn match   simpleNumber           "\<\d\+\(_\d\+\)*[eE][+-]\?\d\+\>"
syn match   simpleNumber           "\<[1-9]\d*\(_\d\+\)*\(\.\(\d\+\(_\d\+\)*\([eE][+-]\?\d\+\)\?\)\?\)\?\>"
syn match   simpleNumber           "\<\(\d\+\(_\d\+\)*\)\?\.\d\+\(_\d\+\)*\([eE][+-]\?\d\+\)\?\>"
syn match   simpleNumber           "\<\d\+\(_\d\+\)*\.\(\d\+\(_\d\+\)*\([eE][+-]\?\d\+\)\?\)\?\>"
syn region  simpleRegexpString     start=+[,(=+]\s*/[^/*]+ms=e-1,me=e-1 skip=+\\\\\|\\/+ end=+/[gimuys]\{0,2\}\s*$+ end=+/[gimuys]\{0,2\}\s*[+;.,)\]}]+me=e-1 end=+/[gimuys]\{0,2\}\s\+\/+me=e-1 contains=@htmlPreproc,simpleComment oneline

syn keyword simpleConditional	if else switch
syn keyword simpleRepeat		while for do in
syn keyword simpleBranch		break continue
syn keyword simpleOperator		new delete instanceof typeof
syn keyword simpleType		Array Boolean Date Function Number Object String RegExp
syn keyword simpleStatement		return with await
syn keyword simpleBoolean		true false
syn keyword simpleNull		null undefined
syn keyword simpleDeclaration	let
syn match   simpleIdentifier    "[a-zA-Z_][a-zA-Z_0-9]*" nextgroup=simpleAssignment
syn match   simpleAssignment    "=" contained nextgroup=simpleValue

syn keyword simpleLabel		case default
syn keyword simpleException		try catch finally throw
syn keyword simpleMessage		alert confirm prompt status
syn keyword simpleGlobal		self window top parent
syn keyword simpleMember		document event location 
syn keyword simpleDeprecated	escape unescape
syn keyword simpleReserved      abstract boolean byte char class const debugger double enum export extends final float goto implements import int interface long native package private protected public short static super synchronized throws transient volatile async

syn cluster  simpleEmbededExpr	contains=simpleBoolean,simpleNull,simpleIdentifier,simpleStringD,simpleStringS,simpleStringT

syn keyword   simpleFunction      fn
syn match     simpleBraces	  "[{}\[\]]"
syn match     simpleParens	  "[()]"

if main_syntax == "simple"
  syn sync fromstart
  syn sync maxlines=100

  syn sync ccomment simpleComment
endif

" Define the default highlighting.
" Only when an item doesn't have highlighting yet
hi def link simpleComment		Comment
hi def link simpleLineComment		Comment
hi def link simpleCommentTodo		Todo
hi def link simpleCommentExpect		Todo
hi def link simpleSpecial		Special
hi def link simpleStringS		String
hi def link simpleStringD		String
hi def link simpleStringT		String
hi def link simpleCharacter		Character
hi def link simpleNumber		Number
hi def link simpleConditional		Conditional
hi def link simpleRepeat		Repeat
hi def link simpleBranch		Conditional
hi def link simpleOperator		Operator
hi def link simpleType			Type
hi def link simpleStatement		Statement
hi def link simpleFunction		Function
hi def link simpleBraces		Function
hi def link simpleError		        Error
hi def link simpleNull			Keyword
hi def link simpleBoolean		Boolean
hi def link simpleRegexpString		String

hi def link simpleIdentifier		Identifier
hi def link simpleDeclaration		Function
hi def link simpleLabel		        Label
hi def link simpleException		Exception
hi def link simpleMessage		Keyword
hi def link simpleGlobal		Keyword
hi def link simpleMember		Keyword
hi def link simpleDeprecated		Exception 
hi def link simpleReserved		Keyword
hi def link simpleDebug		        Debug
hi def link simpleConstant		Label
hi def link simpleEmbed		        Special



let b:current_syntax = "simple"
if main_syntax == 'simple'
  unlet main_syntax
endif
let &cpo = s:cpo_save
unlet s:cpo_save

" vim: ts=8
