#include "grammar.h"
/*
expression     : literal
               | unary
               | binary
               | grouping ;

literal        : NUMBER | STRING | TRUE | FALSE ;

grouping       : LP expression RP ;

// unary       : ( "-" | "!" ) expression ;
//
binary         : expression operator expression ;

operator       : EQUALITY
               | "!="
               | "<"
               | "<="
               | ">"
               | ">="
               | "+"
               | "-"
               | "*"
               | "/" ;
*/

execution_ctx *create_execution_ctx() {
  return calloc(sizeof(execution_ctx), 1);
}
void process_token(execution_ctx *execution_ctx, token token){};
