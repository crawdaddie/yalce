#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Seq Seq;
typedef struct SeqList SeqList;

typedef struct Seq {
  int8_t tag;
  union {
    int32_t int_val;
    SeqList *list;
  } data;
} Seq;

typedef enum {
  SEQ_INT = 0,
  SEQ_LIST = 1,
  SEQ_CHOOSE = 2,
  SEQ_ALT = 3,
} SeqType;

struct SeqList {
  Seq data;
  SeqList *next;
};

typedef struct CSeqList CSeqList;
typedef struct CSeq CSeq;

bool seq_eq(Seq a, Seq b);
bool seql_eq(SeqList *a, SeqList *b) {

  if (a && b && a->next && b->next) {
    return seq_eq(a->data, b->data) && seql_eq(a->next, b->next);
  }

  if (a && b) {
    return seq_eq(a->data, b->data);
  }
  return false;
}

bool seq_eq(Seq a, Seq b) {
  if (a.tag != b.tag) {
    return false;
  }
  switch (a.tag) {
  case SEQ_INT: {
    return a.data.int_val == b.data.int_val;
  }
  case SEQ_LIST:
  case SEQ_CHOOSE:
  case SEQ_ALT: {
    SeqList *la = a.data.list;
    SeqList *lb = b.data.list;
    if (la && lb) {
      return seql_eq(la, lb);
    }

    return false;
  }
  }
  return false;
}

typedef struct CSeqArray {
  int32_t size;
  struct CSeq *data;
} CSeqArray;

typedef struct CSeq {
  SeqType tag;
  union {
    int32_t int_val;
    struct {
      CSeqArray arr;
    } choose;

    struct {
      int32_t state;
      CSeqArray arr;
    } list;

    struct {
      int32_t state;
      CSeqArray arr;
    } alternate;
  } data;
} CSeq;

struct CSeqList {
  CSeq data;
  CSeqList *next;
};

CSeq *compile_data(Seq *seq);
CSeqArray seq_list_to_arr(Seq *seq) {

  SeqList *l = seq->data.list;

  int n = 0;
  while (l != NULL) {
    n++;
    l = l->next;
  }

  CSeq *data = malloc(sizeof(CSeq) * n);
  int i = 0;
  for (l = seq->data.list; l; l = l->next, i++) {
    data[i] = *compile_data(&l->data);
  }
  return (CSeqArray){.size = n, .data = data};
}

CSeq *compile_data(Seq *seq) {
  CSeq *cseq = malloc(sizeof(CSeq));
  int tag = seq->tag;
  cseq->tag = tag;
  switch (tag) {
  case SEQ_INT: {
    cseq->data.int_val = seq->data.int_val;
    break;
  }

  case SEQ_CHOOSE: {
    cseq->data.choose.arr = seq_list_to_arr(seq);
    break;
  }
  case SEQ_LIST: {
    cseq->data.list.arr = seq_list_to_arr(seq);
    cseq->data.list.state = 0;
  }
  case SEQ_ALT: {
    cseq->data.alternate.arr = seq_list_to_arr(seq);
    cseq->data.alternate.state = 0;
    break;
  }
  }
  return cseq;
}

// void print_cseq(CSeq *seq) {
//   if (!seq) {
//     return;
//   }
//
//   switch (seq->tag) {
//   case SEQ_INT: {
//     printf("%d, ", seq->data.int_val);
//
//     break;
//   }
//   case SEQ_ALT: {
//     printf("alternate: <");
//     for (int i = 0; i < seq->data.arr.size; i++) {
//       print_cseq(seq->data.alternate.arr.data + i);
//     }
//     printf(">, ");
//     break;
//   }
//
//   case SEQ_CHOOSE: {
//     printf("choose: {");
//     for (int i = 0; i < seq->data.arr.size; i++) {
//       print_cseq(seq->data.arr.data + i);
//     }
//     printf("}, ");
//
//     break;
//   }
//
//   case SEQ_LIST: {
//     printf("[");
//     for (int i = 0; i < seq->data.arr.size; i++) {
//       print_cseq(seq->data.list.arr.data + i);
//     }
//     printf("], ");
//
//     break;
//   }
//   }
// }
//
typedef struct {
  int8_t tag;
  // Seq seq;
  // int32_t val;
  Seq val;

} CorPromise;

typedef void *(*CorFn)(void *);
typedef struct {
  CSeq *current;
  struct CorState *parent;
} CorState;

typedef struct {
  int32_t counter;
  CorFn fn;
  void *state;
  struct Cor *next;
  CorPromise promise;
} Cor;

void iter_seq(CorState **state_ptr, Seq *val) {
  CorState *state = *state_ptr;

  if (!state || !state->current) {
    return;
  }

  CSeq *current = state->current;
  switch (current->tag) {
  case SEQ_INT: {
    *val = (Seq){.tag = SEQ_INT, .data = {.int_val = current->data.int_val}};
    break;
  }
  case SEQ_LIST: {
    int c = current->data.list.state;

    if (c >= current->data.list.arr.size) {
      current->data.list.state = 0;
      if (state->parent) {
        CorState *parent = (CorState *)state->parent;
        free(state);
        *state_ptr = parent;
        iter_seq(state_ptr, val);
      } else {
        iter_seq(state_ptr, val);
      }
      return;
    }

    CSeq *next_elem = current->data.list.arr.data + c;
    current->data.list.state++;

    if (next_elem->tag == SEQ_INT) {
      *val = (Seq){.tag = SEQ_INT, .data = {.int_val = current->data.int_val}};
    } else {
      CorState *child_state = malloc(sizeof(CorState));
      child_state->current = next_elem;
      child_state->parent = (struct CorState *)state;
      *state_ptr = child_state;
      iter_seq(state_ptr, val);
    }
    break;
  }

  case SEQ_CHOOSE: {

    if (current->data.choose.arr.size == 0) {
      if (state->parent) {
        CorState *parent = (CorState *)state->parent;
        free(state);
        *state_ptr = parent;
        iter_seq(state_ptr, val);
      }
      return;
    }

    int c = rand() % current->data.choose.arr.size;
    CSeq *chosen = current->data.choose.arr.data + c;

    if (chosen->tag == SEQ_INT) {
      *val = (Seq){.tag = SEQ_INT, .data = {.int_val = chosen->data.int_val}};
      if (state->parent) {
        CorState *parent = (CorState *)state->parent;
        free(state);
        *state_ptr = parent;
      }
    } else {
      CorState *child_state = malloc(sizeof(CorState));
      child_state->current = chosen;
      child_state->parent =
          state->parent; // Skip the CHOOSE node, link directly to its parent
      free(state);       // Free the CHOOSE node since we're done with it
      *state_ptr = child_state;
      iter_seq(state_ptr, val);
    }
    break;
  }
  case SEQ_ALT: {

    if (current->data.alternate.arr.size == 0) {
      if (state->parent) {
        CorState *parent = (CorState *)state->parent;
        free(state);
        *state_ptr = parent;
        iter_seq(state_ptr, val);
      }
      return;
    }

    if (current->data.alternate.state >= current->data.alternate.arr.size) {
      current->data.alternate.state = 0;
    }
    int c = current->data.alternate.state++;

    CSeq *chosen = current->data.alternate.arr.data + c;

    if (chosen->tag == SEQ_INT) {
      *val = (Seq){.tag = SEQ_INT, .data = {.int_val = chosen->data.int_val}};
      if (state->parent) {
        CorState *parent = (CorState *)state->parent;
        free(state);
        *state_ptr = parent;
      }
    } else {
      // Nested structure - create child, then pop this CHOOSE node
      CorState *child_state = malloc(sizeof(CorState));
      child_state->current = chosen;
      child_state->parent =
          state->parent; // Skip the CHOOSE node, link directly to its parent
      free(state);       // Free the CHOOSE node since we're done with it
      *state_ptr = child_state;
      iter_seq(state_ptr, val);
    }
    break;
  }
  }
}

void *seq_coroutine_fn(Cor *cor) {
  CorState **state_ptr = (CorState **)&cor->state;
  Seq val;
  iter_seq(state_ptr, &val);
  cor->promise.tag = 0;
  cor->promise.val = val;
  cor->counter++;
  return cor;
}

// int32_t counter;
// CorFn fn;
// void *state;
// struct Cor *next;
// CorPromise promise;

Cor *compile_coroutine_from_seq(Seq seq) {
  // printf("compile seq %d\n", seq.tag);
  CSeq *base = compile_data(&seq);
  CorState *state = malloc(sizeof(CorState));
  *state = (CorState){base, NULL};

  // print_cseq(data);
  Cor *cor = malloc(sizeof(Cor));
  *cor = (Cor){
      0,
      (CorFn)seq_coroutine_fn,
      state,
      NULL,
  };
  return cor;
}
