#ifndef _ENGINE_OSC_H
#define _ENGINE_OSC_H
void maketable_sin(void);
void maketable_sq(void);

#define SQ_TABSIZE (1 << 11)
#define SIN_TABSIZE (1 << 11)
extern double sin_table[SIN_TABSIZE];
extern double sq_table[SQ_TABSIZE];
#endif
