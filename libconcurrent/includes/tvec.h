#include <string.h>

#include "primitives.h"

#ifndef _TVEC_H_
#define _TVEC_H_

#ifdef sparc
#    define bitword_t                  uint32_t
#    define _TVEC_DIVISION_SHIFT_BITS_ 5
#    define _TVEC_MODULO_BITS_         31
#    define _TVEC_BIWORD_SIZE_         32
#else
#    define bitword_t                  uint64_t
#    define _TVEC_DIVISION_SHIFT_BITS_ 6
#    define _TVEC_MODULO_BITS_         63
#    define _TVEC_BIWORD_SIZE_         64
#endif 

/* automatic partial unrolling*/
#define _TVEC_CELLS_(N)               ((N >> _TVEC_DIVISION_SHIFT_BITS_) + 1)
#define _TVEC_VECTOR_SIZE(N)          (_TVEC_CELLS_(N) * sizeof(bitword_t))
#define LOOP(EXPR, I, TIMES)          {for (I = 0; I < TIMES; I++) {EXPR;}}


typedef struct ToggleVector {
    uint32_t nthreads;
    uint32_t tvec_cells;
    bitword_t *cell;
} ToggleVector;


// Operations that handle banks of bits and not the whole vectors
// --------------------------------------------------------------

static inline int TVEC_GET_BANK_OF_BIT(int bit, uint32_t nthreads) {
    if (nthreads > _TVEC_BIWORD_SIZE_)
        return bit >> _TVEC_DIVISION_SHIFT_BITS_;
    else return 0;
}

static inline void TVEC_ATOMIC_COPY_BANKS(ToggleVector *tv1, ToggleVector *tv2, int bank) {
    tv1->cell[bank] = tv2->cell[bank];
}

static inline void TVEC_ATOMIC_ADD_BANK(volatile ToggleVector *tv1, ToggleVector *tv2, int bank) {
#if _TVEC_BIWORD_SIZE_ == 32
    FAA32(&tv1->cell[bank], tv2->cell[bank]);
#else
    FAA64(&tv1->cell[bank], tv2->cell[bank]);
#endif
}

static inline void TVEC_NEGATIVE_BANK(ToggleVector *tv1, ToggleVector *tv2, int bank) {
    tv1->cell[bank] = -tv2->cell[bank];
}

static inline void TVEC_XOR_BANKS(ToggleVector *res, ToggleVector *tv1, ToggleVector *tv2, int bank) {
    res->cell[bank] = tv1->cell[bank] ^ tv2->cell[bank];
}

static inline void TVEC_AND_BANKS(ToggleVector *res, ToggleVector *tv1, ToggleVector *tv2, int bank) {
    res->cell[bank] = tv1->cell[bank] & tv2->cell[bank];
}

// Operations that handle whole vectors of bits
// --------------------------------------------

static inline void TVEC_INIT(ToggleVector *tv1, uint32_t nthreads) {
    int i;

    tv1->nthreads = nthreads;
    tv1->tvec_cells = _TVEC_CELLS_(nthreads);
    tv1->cell = getMemory(_TVEC_VECTOR_SIZE(nthreads));
    LOOP(tv1->cell[i] = 0L, i, tv1->tvec_cells);
}

static inline void TVEC_INIT_AT(ToggleVector *tv1, uint32_t nthreads, void *ptr) {
    int i;

    tv1->nthreads = nthreads;
    tv1->tvec_cells = _TVEC_CELLS_(nthreads);
    tv1->cell = ptr;
    LOOP(tv1->cell[i] = 0L, i, tv1->tvec_cells);
}

static inline void TVEC_SET_ZERO(ToggleVector *tv1) {
    int i;

    LOOP(tv1->cell[i] = 0L, i, tv1->tvec_cells);
}

static inline void TVEC_COPY(ToggleVector *dest, ToggleVector *src) {
    memcpy(dest->cell, src->cell, _TVEC_VECTOR_SIZE(dest->nthreads));
}

static inline void TVEC_NEGATIVE(ToggleVector *res, ToggleVector *tv) {
    int i = 0;

    LOOP(res->cell[i] = -tv->cell[i], i, res->tvec_cells);
}

static inline void TVEC_REVERSE_BIT(ToggleVector *tv1, int bit) {
    int i, offset;

    i = bit >> _TVEC_DIVISION_SHIFT_BITS_;
    offset = bit & _TVEC_MODULO_BITS_;
    tv1->cell[i] ^= ((bitword_t)1) << offset;
}

static inline void TVEC_SET_BIT(ToggleVector *tv1, int bit) {
    int i, offset;

    i = bit >> _TVEC_DIVISION_SHIFT_BITS_;
    offset = bit & _TVEC_MODULO_BITS_;
    tv1->cell[i] |= ((bitword_t)1) << offset;
}

static inline bool TVEC_IS_SET(ToggleVector *tv1, int pid) {
    int i, offset;

    i = pid >> _TVEC_DIVISION_SHIFT_BITS_;
    offset = pid & _TVEC_MODULO_BITS_;
    // Commented code is optimized to avoid branches
    // if ( (tv1.cell[i] & (1 << offset)) ==  0) return false;
    // else return true;
    return (tv1->cell[i] >> offset) & 1;
}

static inline void TVEC_OR(ToggleVector *res, ToggleVector *tv1, ToggleVector *tv2) {
    int i;

    LOOP(res->cell[i] = tv1->cell[i] | tv2->cell[i], i, res->tvec_cells);
}

static inline void TVEC_AND(ToggleVector *res, ToggleVector *tv1, ToggleVector *tv2) {
    int i;

    LOOP(res->cell[i] = tv1->cell[i] & tv2->cell[i], i, res->tvec_cells);
}

static inline void TVEC_XOR(ToggleVector *res, ToggleVector *tv1, ToggleVector *tv2) {
    int i;

    LOOP(res->cell[i] = tv1->cell[i] ^ tv2->cell[i], i, res->tvec_cells);
}

static inline int TVEC_COUNT_BITS(ToggleVector *tv) {
    int i, count;

    count = 0;
    LOOP(count += nonZeroBits(tv->cell[i]), i, tv->tvec_cells);

    return count;
}

#endif


