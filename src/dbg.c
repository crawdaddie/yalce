#include "dbg.h"

void dump_nodes(Node *head, int indent) {
  if (!head) {
    return;
  }
  INDENT(indent);

  write_log("%s num inputs: %d", head->name, (head->num_ins));

  if (head->_sub) {
    write_log("\n");
    dump_nodes(head->_sub, indent + 1);
  }
  if (head->next) {
    write_log("\n");
    dump_nodes(head->next, indent);
  }
  write_log("\n");
  return;
}
