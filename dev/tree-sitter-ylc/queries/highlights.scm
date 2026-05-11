(comment) @comment

[
  "fn"
  "module"
  "let"
  "in"
  "extern"
  "match"
  "with"
  "import"
  "open"
  "type"
  "of"
  "if"
  "then"
  "else"
  "yield"
  "await"
  "for"
  "mut"
] @keyword

[
  "true"
  "false"
] @boolean

[
  (integer)
  (float)
  (double)
] @number

[
  (string)
  (format_string)
  (format_text)
] @string

(char) @string.special
(identifier) @variable
(path_identifier) @string.special.path
(typed_empty_list) @type

(type_declaration name: (identifier) @type)
(type_parameters (identifier) @type.parameter)
(lambda_parameter (identifier) @parameter)
(lambda_argument (identifier) @parameter)

[
  "+"
  "-"
  "*"
  "/"
  "%"
  "|>"
  "@@"
  "&&"
  "||"
  ">"
  "<"
  ">="
  "<="
  "=="
  "!="
  "::"
  ":="
  ":"
  ".."
] @operator

(interpolation
  "{" @punctuation.special
  "}" @punctuation.special)

[
  "("
  ")"
  "["
  "]"
  "[|"
  "|]"
] @punctuation.bracket

[
  ","
  ";"
  "."
] @punctuation.delimiter
