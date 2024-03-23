
#include <stdio.h>
#include <stdlib.h>

// Define Node structure
typedef struct Node {
  int data;
  struct Node *prev;
  struct Node *next;
} Node;

// Define Doubly Linked List structure
typedef struct {
  struct Node *head;
  struct Node *tail;
} DLL;

// Function to create a new node
Node *createNode(int data) {
  Node *newNode = (Node *)malloc(sizeof(Node));
  if (newNode == NULL) {
    printf("Memory allocation failed\n");
    exit(1);
  }
  newNode->data = data;
  newNode->prev = NULL;
  newNode->next = NULL;
  return newNode;
}

// Function to add a node after a particular node
void insertAfter(Node *prevNode, int data) {
  if (prevNode == NULL) {
    printf("Previous node cannot be NULL\n");
    return;
  }
  Node *newNode = createNode(data);
  newNode->next = prevNode->next;
  if (prevNode->next != NULL) {
    prevNode->next->prev = newNode;
  }
  prevNode->next = newNode;
  newNode->prev = prevNode;
}

// Function to remove a particular node from the linked list
void deleteNode(DLL *dll, Node *delNode) {
  if (dll == NULL || delNode == NULL) {
    printf("Invalid arguments\n");
    return;
  }
  if (dll->head == delNode) {
    dll->head = delNode->next;
  }
  if (dll->tail == delNode) {
    dll->tail = delNode->prev;
  }
  if (delNode->prev != NULL) {
    delNode->prev->next = delNode->next;
  }
  if (delNode->next != NULL) {
    delNode->next->prev = delNode->prev;
  }
  free(delNode);
}

// Function to print the doubly linked list
void printDLL(DLL *dll) {
  if (dll == NULL || dll->head == NULL) {
    printf("Doubly linked list is empty\n");
    return;
  }
  Node *temp = dll->head;
  while (temp != NULL) {
    printf("%d ", temp->data);
    temp = temp->next;
  }
  printf("\n");
}

int main_() {
  // Initialize an empty doubly linked list
  DLL *dll = (DLL *)malloc(sizeof(DLL));
  if (dll == NULL) {
    printf("Memory allocation failed\n");
    return 1;
  }
  dll->head = NULL;
  dll->tail = NULL;

  // Insert some nodes
  insertAfter(dll->head, 10);       // Inserts 10 as the first node
  insertAfter(dll->head, 20);       // Inserts 20 after the first node
  insertAfter(dll->head->next, 30); // Inserts 30 after the second node

  // Print the doubly linked list
  printf("Doubly linked list: ");
  printDLL(dll);

  // Delete a node (e.g., the second node)
  deleteNode(dll, dll->head->next);

  // Print the doubly linked list after deletion
  printf("Doubly linked list after deletion: ");
  printDLL(dll);

  // Free the memory allocated for the doubly linked list
  Node *temp;
  while (dll->head != NULL) {
    temp = dll->head;
    dll->head = dll->head->next;
    free(temp);
  }
  free(dll);

  return 0;
}
