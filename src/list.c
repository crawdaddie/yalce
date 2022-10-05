typedef struct list_node {
  void *value;
  struct list_node *next;
  struct list_node *prev;
  struct list_node *tail;
  struct list_node *head;
} list_node;

list_node *append(void *val, list_node *list) {}
list_node *prepend(void *val, list_node *list) {}
void remove(list_node *node) {}
