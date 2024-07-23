if !exists("main_syntax")
  " quit when a syntax file was already loaded
  if exists("b:current_syntax")
    finish
  endif
  let main_syntax = 'ylc'
  set tabstop=2
  set shiftwidth=2
elseif exists("b:current_syntax") && b:current_syntax == "ylc"
  finish
endif

let s:cpo_save = &cpo
set cpo&vim


syn keyword ylcCommentTodo      TODO FIXME XXX TBD contained
syn keyword ylcCommentExpect    expect contained
syn match   ylcLineComment      "#.*$" contains=@Spell,ylcCommentTodo,ylcCommentExpect
syn match   ylcCommentSkip      "^[ \t]*\*\($\|[ \t]\+\)"
syn region  ylcComment	        start="/\*"  end="\*/" contains=@Spell,ylcCommentTodo
syn match   ylcSpecial	        "\\\d\d\d\|\\."
syn region  ylcStringD	        start=+"+  skip=+\\\\\|\\"+  end=+"\|$+	contains=ylcSpecial,@htmlPreproc
syn region  ylcStringD	        start=+`+  skip=+\\\\\|\\`+  end=+`\|$+	contains=ylcSpecial,@htmlPreproc

" syn region  ylcEmbed	          start=+${+  end=+}+	contains=@ylcEmbededExpr
syn region  ylcCharacter  start=+'+ end=+'+ contains=ylcSpecial

" number handling by Christopher Leonard chris.j.leonard@gmx.com
syn match   ylcNumber       "\<0[bB][0-1]\+\(_[0-1]\+\)*\>"
syn match   ylcNumber       "\<0[oO][0-7]\+\(_[0-7]\+\)*\>"
syn match   ylcNumber       "\<0\([0-7]\+\(_[0-7]\+\)*\)\?\>"
syn match   ylcNumber       "\<0[xX][0-9a-fA-F]\+\(_[0-9a-fA-F]\+\)*\>"
syn match   ylcNumber       "\<\d\+\(_\d\+\)*[eE][+-]\?\d\+\>"
syn match   ylcNumber       "\<[1-9]\d*\(_\d\+\)*\(\.\(\d\+\(_\d\+\)*\([eE][+-]\?\d\+\)\?\)\?\)\?\>"
syn match   ylcNumber       "\<\(\d\+\(_\d\+\)*\)\?\.\d\+\(_\d\+\)*\([eE][+-]\?\d\+\)\?\>"
syn match   ylcNumber       "\<\d\+\(_\d\+\)*\.\(\d\+\(_\d\+\)*\([eE][+-]\?\d\+\)\?\)\?\>"
syn region  ylcRegexpString start=+[,(=+]\s*/[^/*]+ms=e-1,me=e-1 skip=+\\\\\|\\/+ end=+/[gimuys]\{0,2\}\s*$+ end=+/[gimuys]\{0,2\}\s*[+;.,)\]}]+me=e-1 end=+/[gimuys]\{0,2\}\s\+\/+me=e-1 contains=@htmlPreproc,ylcComment oneline

syn keyword ylcConditional  if else switch
syn keyword ylcRepeat	    while for do in
syn keyword ylcBranch 	    break continue
syn keyword ylcOperator	    new delete instanceof typeof
syn keyword ylcType         Array Boolean Date Function Number Object String RegExp
syn keyword ylcStatement    return with await
syn keyword ylcBoolean	    true false
syn keyword ylcNull	    null undefined
syn keyword ylcDeclaration  let
syn match   ylcIdentifier   "[a-zA-Z_][a-zA-Z_0-9]*" nextgroup=ylcAssignment
syn match   ylcAssignment   "=" contained nextgroup=ylcValue

syn keyword ylcLabel	    case default
syn keyword ylcException    try catch finally throw
syn keyword ylcMessage	    alert confirm prompt status
syn keyword ylcGlobal	    self window top parent
syn keyword ylcMember	    document event location 
syn keyword ylcDeprecated   escape unescape
syn keyword ylcReserved     abstract boolean byte char class const debugger double enum export extends final float goto implements import int interface long native package private protected public short static super synchronized throws transient volatile async

syn cluster ylcEmbededExpr  contains=ylcBoolean,ylcNull,ylcIdentifier,ylcStringD,ylcStringS,ylcStringT

syn keyword  ylcFunction    fn
syn match    ylcBraces	    "[{}\[\]]"
syn match    ylcParens	    "[()]"

if main_syntax == "ylc"
  syn sync fromstart
  syn sync maxlines=100

  syn sync ccomment ylcComment
endif

" Define the default highlighting.
" Only when an item doesn't have highlighting yet
hi def link ylcComment		    Comment
hi def link ylcLineComment	    Comment
hi def link ylcCommentTodo    	    Todo
hi def link ylcCommentExpect        Todo
hi def link ylcSpecial		    Special
hi def link ylcStringS		    String
hi def link ylcStringD		    String
hi def link ylcStringT		    String
hi def link ylcCharacter	    Character
hi def link ylcNumber		    Number
hi def link ylcConditional	    Conditional
hi def link ylcRepeat		    Repeat
hi def link ylcBranch		    Conditional
hi def link ylcOperator		    Operator
hi def link ylcType		    Type
hi def link ylcStatement	    Statement
hi def link ylcFunction		    Function
hi def link ylcBraces		    Function
hi def link ylcError		    Error
hi def link ylcNull	    	    Keyword
hi def link ylcBoolean		    Boolean
hi def link ylcRegexpString   	    String

hi def link ylcIdentifier     	    Identifier
hi def link ylcDeclaration    	    Function
hi def link ylcLabel		    Label
hi def link ylcException            Exception
hi def link ylcMessage		    Keyword
hi def link ylcGlobal		    Keyword
hi def link ylcMember		    Keyword
hi def link ylcDeprecated	    Exception 
hi def link ylcReserved		    Keyword
hi def link ylcDebug		    Debug
hi def link ylcConstant		    Label
hi def link ylcEmbed		    Special



let b:current_syntax = "ylc"
if main_syntax == 'ylc'
  unlet main_syntax
endif
let &cpo = s:cpo_save
unlet s:cpo_save

" vim: ts=8
