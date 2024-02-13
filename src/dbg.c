#include "dbg.h"

void dump_nodes(Node *head, int indent) {
  if (!head) {
    return;
  }
  INDENT(indent);

  printf("%s num inputs: %d", head->name, (head->num_ins));

  if (head->_sub) {
    printf("\n");
    dump_nodes(head->_sub, indent + 1);
  }
  if (head->next) {
    printf("\n");
    dump_nodes(head->next, indent);
  }
  printf("\n");
  return;
}
