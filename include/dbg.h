#ifndef _DBG_H
#define _DBG_H
#include "log.h"
#include "node.h"
#include <stdio.h>
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define INDENT(indent) write_log("%.*s", indent, "  ")

void dump_nodes(Node *head, int indent);

#endif
