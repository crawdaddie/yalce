const PREC = {
  field: 15,
  subscript: 14,
  application: 13,
  pipe: 12,
  double_at: 11,
  or: 10,
  and: 9,
  compare: 8,
  additive: 7,
  multiplicative: 6,
  cons: 5,
  range: 4,
  pair: 3,
  assignment: 2,
};

module.exports = grammar({
  name: "ylc",

  extras: ($) => [/\s/, $.comment],

  word: ($) => $.identifier,

  conflicts: ($) => [
    [$.destructuring_tuple, $.tuple],
    [$.statement_sequence, $.expression_list],
    [$.fn_signature],
    [$.type_expression],
    [$.type_expression_no_tuple],
    [$.tuple_type],
    [$.type_expression, $.tuple_type],
    [$.type_expression, $.type_expression_no_tuple],
    [$.tuple_type, $.type_expression_no_tuple],
    [$.let_binding, $.type_expression_no_tuple],
    [$.let_binding, $.void],
    [$.let_binding, $.operator_identifier_expression],
    [$.simple_expression, $.let_binding],
    [$.simple_expression, $.lambda_argument],
    [$.simple_expression, $.let_binding, $.lambda_argument],
    [$.simple_expression, $.operator_identifier_expression],
    [$.simple_expression, $.let_binding, $.operator_identifier_expression],
    [$.simple_expression, $.let_binding, $.parenthesized_let_lambda, $.lambda_argument],
  ],

  rules: {
    source_file: ($) => optional($.statement_sequence),

    statement_sequence: ($) =>
      prec.left(seq(
        $.expression,
        repeat(seq(";", $.expression)),
        optional(";"),
      )),

    expression: ($) =>
      choice(
        $.yield_expression,
        $.await_expression,
        $.let_binding,
        $.match_expression,
        $.if_expression,
        $.type_declaration,
        $.for_expression,
        $.thunk_expression,
        $.assignment_expression,
        $.binary_expression,
        $.application_expression,
        $.subscript_expression,
        $.slice_expression,
        $.field_expression,
        $.atom_expression,
      ),

    atom_expression: ($) => $.simple_expression,

    simple_expression: ($) =>
      choice(
        $.integer,
        $.double,
        $.float,
        $.string,
        $.format_string,
        $.char,
        $.true,
        $.false,
        $.void,
        $.identifier,
        $.typed_empty_list,
        $.list,
        $.array,
        $.tuple,
        $.parenthesized_expression,
        $.parenthesized_lambda,
        $.parenthesized_let_lambda,
        $.operator_identifier_expression,
      ),

    yield_expression: ($) => prec.right(seq("yield", $.expression)),
    await_expression: ($) => prec.right(seq("await", $.expression)),
    thunk_expression: ($) => prec.right(seq("\\", $.expression)),

    field_expression: ($) =>
      prec.left(PREC.field, seq($.expression, ".", $.identifier)),

    subscript_expression: ($) =>
      prec.left(PREC.subscript, seq($.expression, "[", $.expression, "]")),

    slice_expression: ($) =>
      prec.left(PREC.subscript, seq($.expression, "[", $.expression, "..", "]")),

    application_expression: ($) =>
      choice(
        prec.left(PREC.double_at, seq($.expression, "@@", $.expression)),
        prec.left(PREC.application, seq($.expression, $.atom_expression)),
      ),

    binary_expression: ($) =>
      choice(
        prec.left(PREC.pipe, seq($.expression, "|>", $.expression)),
        prec.left(PREC.or, seq($.expression, "||", $.expression)),
        prec.left(PREC.and, seq($.expression, "&&", $.expression)),
        prec.left(PREC.compare, seq($.expression, choice(">", "<", ">=", "<=", "==", "!="), $.expression)),
        prec.left(PREC.additive, seq($.expression, choice("+", "-"), $.expression)),
        prec.left(PREC.multiplicative, seq($.expression, choice("*", "/", "%"), $.expression)),
        prec.right(PREC.cons, seq($.expression, "::", $.expression)),
        prec.left(PREC.range, seq($.expression, "..", $.expression)),
        prec.right(PREC.pair, seq($.expression, ":", $.expression)),
      ),

    assignment_expression: ($) =>
      prec.right(PREC.assignment, seq($.expression, ":=", $.expression)),

    let_binding: ($) =>
      prec.right(choice(
        seq("let", "test", "=", field("value", $.expression)),
        seq("let", optional("mut"), field("pattern", $.binding_pattern), "=", field("value", $.expression), "in", field("body", $.expression)),
        seq("let", optional("mut"), field("pattern", $.binding_pattern), "=", field("value", $.expression)),
        seq("let", field("name", $.identifier), "=", "extern", "fn", field("signature", $.fn_signature)),
        seq("let", "()", "=", field("value", $.expression)),
        seq("import", choice($.path_identifier, $.identifier)),
        seq("open", choice($.path_identifier, $.identifier)),
        seq("let", "(", field("operator", $.identifier), ")", "=", field("value", $.expression)),
        seq("let", field("name", $.identifier), ":", field("trait", $.identifier), "=", field("value", $.lambda_expression)),
        $.lambda_expression,
      )),

    binding_pattern: ($) =>
      choice(
        $.lambda_argument,
        $.expression_list,
      ),

    lambda_expression: ($) =>
      choice(
        seq("fn", field("parameters", $.lambda_parameters), "->", field("body", $.statement_sequence), ";"),
        seq("fn", "()", "->", field("body", $.statement_sequence), ";"),
        seq("module", field("parameters", $.lambda_parameters), "->", field("body", $.statement_sequence), ";"),
        seq("module", "()", "->", field("body", $.statement_sequence), ";"),
      ),

    parenthesized_lambda: ($) =>
      choice(
        seq("(", "fn", field("parameters", $.lambda_parameters), "->", field("body", $.statement_sequence), ")"),
        seq("(", "fn", "()", "->", field("body", $.statement_sequence), ")"),
      ),

    parenthesized_let_lambda: ($) =>
      choice(
        seq("(", "let", field("name", $.identifier), "=", "fn", field("parameters", $.lambda_parameters), "->", field("body", $.statement_sequence), ")"),
        seq("(", "let", field("name", $.identifier), "=", "fn", "()", "->", field("body", $.statement_sequence), ")"),
      ),

    lambda_parameters: ($) =>
      seq(
        $.lambda_parameter,
        repeat($.lambda_parameter),
      ),

    lambda_parameter: ($) =>
      choice(
        seq($.lambda_argument, "=", $.expression),
        seq($.lambda_argument, ":", "(", $.type_expression, ")"),
        $.lambda_argument,
      ),

    lambda_argument: ($) =>
      choice(
        $.identifier,
        "_",
        $.destructuring_tuple,
        prec.right(seq($.identifier, "::", $.lambda_argument)),
      ),

    destructuring_tuple: ($) => seq("(", $.expression_list, ")"),

    for_expression: ($) =>
      seq(
        "for",
        field("name", $.identifier),
        "=",
        field("start", $.expression),
        "in",
        field("body", $.expression),
      ),

    match_expression: ($) =>
      seq(
        "match",
        field("value", $.expression),
        "with",
        field("branches", $.match_branches),
      ),

    if_expression: ($) =>
      prec.right(choice(
        seq("if", field("condition", $.expression), "then", field("consequence", $.expression), "else", field("alternative", $.expression)),
        seq("if", field("condition", $.expression), "then", field("consequence", $.expression)),
      )),

    match_branches: ($) => prec.left(repeat1($.match_branch)),

    match_branch: ($) =>
      seq(
        "|",
        field("pattern", choice("_", $.match_test_clause)),
        "->",
        field("body", $.expression),
      ),

    match_test_clause: ($) =>
      choice(
        seq($.expression, "if", $.expression),
        $.expression,
      ),

    list: ($) =>
      seq(
        "[",
        optional($.expression_list),
        optional(","),
        "]",
      ),

    array: ($) =>
      seq(
        "[|",
        optional($.expression_list),
        optional(","),
        "|]",
      ),

    tuple: ($) =>
      seq(
        "(",
        $.expression_list,
        optional(","),
        ")",
      ),

    parenthesized_expression: ($) => seq("(", $.statement_sequence, ")"),

    expression_list: ($) =>
      prec.left(seq(
        $.expression,
        repeat(seq(",", $.expression)),
      )),

    type_declaration: ($) =>
      prec.right(choice(
        seq("type", field("name", $.identifier), "=", field("body", $.type_expression)),
        seq("type", field("name", $.identifier)),
        seq("type", field("parameters", $.type_parameters), "=", field("body", $.type_expression)),
      )),

    type_parameters: ($) =>
      seq(
        $.identifier,
        $.identifier,
        repeat($.identifier),
      ),

    fn_signature: ($) =>
      prec.right(
        seq(
          $.type_expression,
          "->",
          $.type_expression,
          repeat(seq("->", $.type_expression)),
        ),
      ),

    type_expression: ($) =>
      choice(
        $.tuple_type,
        $.type_expression_no_tuple,
      ),

    tuple_type: ($) =>
      prec.left(seq(
        $.type_expression_no_tuple,
        ",",
        $.type_expression_no_tuple,
        repeat(seq(",", $.type_expression_no_tuple)),
      )),

    type_expression_no_tuple: ($) =>
      prec.left(choice(
        $.type_atom,
        prec.left(seq("|", $.type_atom)),
        prec.left(seq($.type_expression_no_tuple, "|", $.type_atom)),
        $.fn_signature,
      )),

    type_atom: ($) =>
      prec.right(choice(
        $.identifier,
        prec.right(seq($.identifier, "=", $.integer)),
        seq($.identifier, "of", $.type_atom),
        prec.right(PREC.pair, seq($.identifier, ":", $.type_expression_no_tuple)),
        seq("(", $.type_expression, ")"),
        seq("(", $.type_expression_no_tuple, ",", ")"),
        seq("(", $.tuple_type, ",", ")"),
        $.void,
        prec.left(PREC.field, seq($.identifier, ".", $.identifier)),
      )),

    format_string: ($) =>
      choice(
        seq("`", repeat(choice($.format_text, $.interpolation)), "`"),
        seq("```", repeat(choice($.format_text, $.interpolation)), "```"),
      ),

    interpolation: ($) => seq("{", $.expression, "}"),

    comment: () => token(seq("#", /.*/)),

    identifier: () => token(choice(
      /[A-Za-z_][A-Za-z0-9_]*/,
      /[$*+\-/=>@^|%<~][&$*+\-/=>@^|%<!.:?~]*/,
    )),

    path_identifier: () => token(/(\.\.\/)*[A-Za-z_][A-Za-z0-9_\/]*/),
    typed_empty_list: () => token(/[A-Za-z_][A-Za-z0-9_]*\[\]/),

    integer: () => token(choice(/-?0/, /-?[1-9][0-9]*/)),
    float: () => token(/-?[0-9]+(\.[0-9]*)?f/),
    double: () => token(/-?[0-9]*\.([0-9]*[1-9]|[0-9][0-9]*)?/),
    string: () => token(seq('"', repeat(choice(/[^"\\\n]+/, /\\./)), '"')),
    char: () => token(seq("'", choice(/[^'\\\n]/, /\\./), "'")),
    format_text: () => token(prec(1, /[^`{}\\]+|\\./)),

    true: () => "true",
    false: () => "false",
    void: () => "()",

    operator_identifier_expression: ($) =>
      seq(
        "(",
        choice("+", "-", "*", "/", "%", "<", ">", "&&", "||", ">=", "<=", "!=", "==", "|>", ":", "::", $.identifier),
        ")",
      ),
  },
});
