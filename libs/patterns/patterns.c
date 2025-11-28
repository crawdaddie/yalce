#include "ylc_datatypes.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Seq Seq;
typedef struct SeqList SeqList;

YLC_STRING_TYPE(String)

typedef struct Seq {
  int8_t tag;
  union { // Union starts at offset 4
    int32_t int_val;
    double num_val;
    String key_val;
    SeqList *list;
  } data;
} __attribute__((packed)) Seq;

typedef enum {
  SEQ_INT = 0,
  SEQ_NUM = 1,
  SEQ_KEY = 2,
  SEQ_LIST = 3,
  SEQ_CHOOSE = 4,
  SEQ_ALT = 5,
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
    double num_val;
    char *key_val;
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

// CSeqArray __seq_list_to_arr(Seq *seq) {
//
//   printf("seq_list_to_arr called with tag=%d\n", seq->tag);
//   printf("  sizeof(Seq) = %zu\n", sizeof(Seq));
//   printf("  raw i128 value: 0x%016llx%016llx\n",
//          (unsigned long long)(seq->data.raw >> 64),
//          (unsigned long long)(seq->data.raw & 0xFFFFFFFFFFFFFFFFULL));
//
//   // Dump the raw bytes of the struct
//   printf("  raw bytes: ");
//   unsigned char *bytes = (unsigned char *)seq;
//   for (int i = 0; i < sizeof(Seq); i++) {
//     printf("%02x ", bytes[i]);
//   }
//   printf("\n");
//
//   SeqList *l = seq->data.list;
//   printf("  list pointer (direct): %p\n", (void *)l);
//
//   // Try extracting pointer from lower 64 bits of i128
//   uint64_t ptr_val_lower = (uint64_t)(seq->data.raw & 0xFFFFFFFFFFFFFFFFULL);
//   printf("  pointer from lower 64 bits: %p\n", (void *)ptr_val_lower);
//
//   // Try extracting pointer from upper 64 bits of i128
//   uint64_t ptr_val_upper = (uint64_t)(seq->data.raw >> 64);
//   printf("  pointer from upper 64 bits: %p\n", (void *)ptr_val_upper);
//
//   int n = 0;
//   while (l != NULL) {
//     printf("  element %d: tag=%d, next=%p\n", n, l->data.tag, (void
//     *)l->next); n++; l = l->next;
//   }
//
//   CSeq *data = malloc(sizeof(CSeq) * n);
//   int i = 0;
//   for (l = seq->data.list; l; l = l->next, i++) {
//     data[i] = *compile_data(&l->data);
//   }
//   return (CSeqArray){.size = n, .data = data};
// }
//
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
    printf("compile data int %d seq->data.int_val %d\n", cseq->data.int_val,
           seq->data.int_val);
    break;
  }

  case SEQ_NUM: {
    cseq->data.num_val = seq->data.num_val;
    break;
  }

  case SEQ_KEY: {
    char *copy = malloc(sizeof(char) * seq->data.key_val.size + 1);
    memcpy(copy, seq->data.key_val.chars, seq->data.key_val.size);
    copy[seq->data.key_val.size] = '\0';

    cseq->data.key_val = copy;
    break;
  }

  case SEQ_CHOOSE: {
    cseq->data.choose.arr = seq_list_to_arr(seq);
    break;
  }
  case SEQ_LIST: {
    cseq->data.list.arr = seq_list_to_arr(seq);
    cseq->data.list.state = 0;
    break;
  }
  case SEQ_ALT: {
    cseq->data.alternate.arr = seq_list_to_arr(seq);
    cseq->data.alternate.state = 0;
    break;
  }
  }
  return cseq;
}

void print_cseq(CSeq *seq) {
  if (!seq) {
    return;
  }

  switch (seq->tag) {
  case SEQ_INT: {
    printf("%d, ", seq->data.int_val);
    break;
  }

  case SEQ_NUM: {
    printf("%f, ", seq->data.num_val);
    break;
  }

  case SEQ_KEY: {
    printf("%s, ", seq->data.key_val);
    break;
  }
  case SEQ_ALT: {
    printf("alternate: <");
    for (int i = 0; i < seq->data.alternate.arr.size; i++) {
      print_cseq(seq->data.alternate.arr.data + i);
    }
    printf(">, ");
    break;
  }

  case SEQ_CHOOSE: {
    printf("choose: {");
    for (int i = 0; i < seq->data.choose.arr.size; i++) {
      print_cseq(seq->data.choose.arr.data + i);
    }
    printf("}, ");

    break;
  }

  case SEQ_LIST: {
    printf("[");
    for (int i = 0; i < seq->data.list.arr.size; i++) {
      print_cseq(seq->data.list.arr.data + i);
    }
    printf("], ");

    break;
  }
  }
}
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

void set_val(CSeq *current, Seq *val) {
  switch (current->tag) {
  case SEQ_INT: {

    printf("set int\n");
    *val = (Seq){.tag = SEQ_INT, .data = {.int_val = current->data.int_val}};
    break;
  }

  case SEQ_NUM: {

    printf("set num\n");
    *val = (Seq){.tag = SEQ_NUM, .data = {.num_val = current->data.num_val}};

    break;
  }

  case SEQ_KEY: {

    printf("set key\n");
    *val = (Seq){
        .tag = SEQ_KEY,
        .data = {.key_val = (String){.size = strlen(current->data.key_val),
                                     .chars = current->data.key_val}}};

    break;
  }
  }
}
void iter_seq(CorState **state_ptr, Seq *val) {

  CorState *state = *state_ptr;

  if (!state || !state->current) {
    return;
  }

  CSeq *current = state->current;
  printf("iter seq %d\n", current->tag);
  switch (current->tag) {
  case SEQ_INT: {
    printf("set int\n");
    *val = (Seq){.tag = SEQ_INT, .data = {.int_val = current->data.int_val}};
    break;
  }

  case SEQ_NUM: {
    *val = (Seq){.tag = SEQ_NUM, .data = {.num_val = current->data.num_val}};
    break;
  }

  case SEQ_KEY: {
    *val = (Seq){
        .tag = SEQ_KEY,
        .data = {.key_val = (String){.chars = current->data.key_val,
                                     .size = strlen(current->data.key_val)}}};
    break;
  }
  case SEQ_LIST: {
    int c = current->data.list.state;
    printf("list state %d\n", c);

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

    if (next_elem->tag == SEQ_INT || next_elem->tag == SEQ_NUM ||
        next_elem->tag == SEQ_KEY) {
      set_val(current, val);
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

    if (chosen->tag == SEQ_INT || chosen->tag == SEQ_NUM ||
        chosen->tag == SEQ_KEY) {
      set_val(chosen, val);

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

    if (chosen->tag == SEQ_INT || chosen->tag == SEQ_NUM ||
        chosen->tag == SEQ_KEY) {
      set_val(chosen, val);
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
  if (val.tag == SEQ_INT) {
    printf("yield (int) %d\n", val.data.int_val);
  } else if (val.tag == SEQ_NUM) {

    printf("yield (num) %f\n", val.data.num_val);
  } else if (val.tag == SEQ_KEY) {

    printf("yield (key) %s\n", val.data.key_val.chars);
  }
  cor->counter++;
  return cor;
}

// int32_t counter;
// CorFn fn;
// void *state;
// struct Cor *next;
// CorPromise promise;

void print_seq(Seq *seq) {
  if (!seq) {
    return;
  }

  switch (seq->tag) {
  case SEQ_INT: {
    printf("%d, ", seq->data.int_val);
    break;
  }

  case SEQ_NUM: {
    printf("%f, ", seq->data.num_val);
    break;
  }

  case SEQ_KEY: {
    printf("%s, ", seq->data.key_val);
    break;
  }
  case SEQ_ALT: {
    printf("alternate: <");
    for (SeqList *l = seq->data.list; l; l = l->next) {
      print_seq(&l->data);
    }
    printf(">, ");
    break;
  }

  case SEQ_CHOOSE: {
    printf("choose: {");

    for (SeqList *l = seq->data.list; l; l = l->next) {
      print_seq(&l->data);
    }
    printf("}, ");

    break;
  }

  case SEQ_LIST: {
    printf("[");

    for (SeqList *l = seq->data.list; l; l = l->next) {
      print_seq(&l->data);
    }
    printf("], ");

    break;
  }
  }
}

Cor *compile_coroutine_from_seq(Seq *seq) {
  print_seq(seq);
  // CSeq *base = compile_data(seq);
  // CorState *state = malloc(sizeof(CorState));
  // *state = (CorState){base, NULL};

  Cor *cor = malloc(sizeof(Cor));
  *cor = (Cor){
      0,
      (CorFn)seq_coroutine_fn,
      // state,
      NULL,
  };
  return cor;
}

// Unions for type punning
typedef union {
  uint8_t bytes[4];
  float vfloat;
  int32_t vint;
} conv32_t;

typedef union {
  uint8_t bytes[8];
  uint8_t bytes4[4];
  double vdouble;
  float vfloat;
  int64_t vint64;
} conv64_t;

// Heap-allocated versions (caller must free)
uint8_t *double_to_bytes_heap(double d) {
  uint8_t *result = malloc(8);
  conv64_t conv;
  conv.vdouble = d;
  memcpy(result, conv.bytes, 8);
  return result;
}

uint8_t *int_to_bytes_heap(int32_t i) {
  uint8_t *result = malloc(4);
  conv32_t conv;
  conv.vint = i;
  memcpy(result, conv.bytes, 4);
  return result;
}

uint8_t *float_to_bytes_heap(float f) {
  uint8_t *result = malloc(4);
  conv32_t conv;
  conv.vfloat = f;
  memcpy(result, conv.bytes, 4);
  return result;
}

// Buffer-based versions (caller provides buffer)
void double_to_bytes(double d, uint8_t *buffer) {
  conv64_t conv;
  conv.vdouble = d;
  memcpy(buffer, conv.bytes, 8);
}

void int_to_bytes(int32_t i, uint8_t *buffer) {
  conv32_t conv;
  conv.vint = i;
  memcpy(buffer, conv.bytes, 4);
}

// Alternative: direct memcpy (avoids union, more portable)
void double_to_bytes_memcpy(double d, uint8_t *buffer) {
  memcpy(buffer, &d, 8);
}

void int_to_bytes_memcpy(int32_t i, uint8_t *buffer) { memcpy(buffer, &i, 4); }

void encode_int32_as_bytes(int32_t d, uint8_t *buffer) {
  conv32_t conv;
  conv.vint = d;
  memcpy(buffer, conv.bytes, 4);
}

double int32_bytes_to_int(uint8_t *buffer) {
  conv32_t conv;
  memcpy(conv.bytes, buffer, 4);
  return (int)conv.vint;
}

void encode_double_as_float_bytes(double d, uint8_t *buffer) {
  conv32_t conv;
  conv.vfloat = (float)d;
  memcpy(buffer, conv.bytes, 4);
}

double float_bytes_to_double(uint8_t *buffer) {
  conv64_t conv;
  memcpy(conv.bytes, buffer, 4);
  return (double)conv.vfloat;
}

void encode_double_as_bytes(double d, uint8_t *buffer) {
  conv64_t conv;
  conv.vdouble = d;
  memcpy(buffer, conv.bytes, 8);
}
