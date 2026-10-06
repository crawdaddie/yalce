if exists('b:current_syntax')
  finish
endif

syntax match trackerCsvNoteToken /\<\x\x[A-Ga-g]\%([#_-]\)\=\d/ contains=trackerCsvInstrument,trackerCsvMidiNote
syntax match trackerCsvInstrument /\<\x\x/ contained
syntax match trackerCsvMidiNote /[A-Ga-g]\%([#_-]\)\=\d/ contained

" Parameters are standalone command characters followed by two hex digits.
syntax match trackerCsvCommand /\%(^\|[[:space:];()]\)\zs[A-Za-z]\x\x\>/ contains=trackerCsvCommandId,trackerCsvCommandValue
syntax match trackerCsvCommandId /[A-Za-z]/ contained
syntax match trackerCsvCommandValue /\x\x\>/ contained

highlight default link trackerCsvInstrument Type
highlight default link trackerCsvMidiNote Identifier
highlight default link trackerCsvCommandId Keyword
highlight default link trackerCsvCommandValue Number

let b:current_syntax = 'tracker_csv'
