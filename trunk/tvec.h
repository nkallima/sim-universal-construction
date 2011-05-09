#include "primitives.h"

#ifndef _TVEC_H_
#define _TVEC_H_

/* automatic partial unroling*/
#if N_THREADS <= 0
#   error Unacceptable N_THREADS size
#elif N_THREADS <= 63
#   define _TOGGLE_VEC_CELLS_         1
#   define LOOP(EXPR, I)              {I=0;EXPR;}
#elif N_THREADS <= 126
#   define _TOGGLE_VEC_CELLS_         2
#   define LOOP(EXPR, I)              {I=0;EXPR; I++; EXPR;}
#elif N_THREADS <= 189
#   define _TOGGLE_VEC_CELLS_         3
#   define LOOP(EXPR, I)              {I=0;EXPR; I++; EXPR; I++; EXPR;}
#else
#   define _TOGGLE_VEC_CELLS_         ((N_THREADS / 63) + (N_THREADS % 63))
#   define LOOP(EXPR, I)              {for (I = 0; I < _TOGGLE_VEC_CELLS_; I++) {EXPR;}}
#endif


typedef struct ToggleVector {
    int64_t cell[_TOGGLE_VEC_CELLS_];
} ToggleVector;

inline static void TVEC_ATOMIC_ADD(volatile ToggleVector *tv1, ToggleVector tv2) {
#if N_THREADS > 63
    int i = 0;

    LOOP(if (tv2.cell[i] != 0) FAA(&tv1->cell[i], tv2.cell[i]);, i);
#else
    FAA(&tv1->cell[0], tv2.cell[0]);
#endif
}

inline static ToggleVector TVEC_NEGATIVE(ToggleVector tv) {
    int i = 0;
    ToggleVector res;

    LOOP(res.cell[i] = -tv.cell[i], i);
    return res;
}


inline static void TVEC_REVERSE_BIT(ToggleVector *tv1, int bit) {
#if N_THREADS > 63
    int i, offset;

    i = bit/63;
    offset = bit%63;
    tv1->cell[i] ^= 1L << offset;
#else
    tv1->cell[0] ^= 1L << bit;
#endif
}


inline static void TVEC_SET_BIT(ToggleVector *tv1, int bit) {
#if N_THREADS > 63
    int i, offset;

    i = bit / 63;
    offset = bit % 63;
    tv1->cell[i] |= 1L << offset;
#else
    tv1->cell[0] |= 1L << bit;
#endif
}


inline static void TVEC_SET_ZERO(ToggleVector *tv1) {
    int i;

    LOOP(tv1->cell[i] = 0L, i);
}


inline static bool TVEC_IS_SET(ToggleVector tv1, int64_t pid) {
#if N_THREADS > 63
    int64_t i, offset;

    i = pid / 63;
    offset = pid % 63;
    if ((tv1.cell[i] & (1L << offset)) == 0) return false;
    else return true;
#else
    if ((tv1.cell[0] & (1L << pid)) == 0) return false;
    else return true;
#endif
}


inline static ToggleVector TVEC_OR(ToggleVector tv1, ToggleVector tv2) {
    int i;
    ToggleVector res;

    LOOP(res.cell[i] = tv1.cell[i] | tv2.cell[i], i);
    return res;
}


inline static ToggleVector TVEC_AND(ToggleVector tv1, ToggleVector tv2) {
    int i;
    ToggleVector res;

    LOOP(res.cell[i] = tv1.cell[i] & tv2.cell[i], i);
    return res;
}


inline static ToggleVector TVEC_XOR(ToggleVector tv1, ToggleVector tv2) {
    int i;
    ToggleVector res;

    LOOP(res.cell[i] = tv1.cell[i] ^ tv2.cell[i], i);
    return res;
}


int TVEC_SEARCH_FIRST_BIT(ToggleVector tv) {
    int64_t pos = 0;
#if N_THREADS > 63
    int i = 0;

    LOOP( if(tv.cell[i] != 0) {bitSearchFirst(pos, tv.cell[i]); return i * 63 + pos;}, i);
    return -1;
#else
    bitSearchFirst(pos, tv.cell[0]);   
    return pos;
#endif
}

inline static int TVEC_COUNT_BITS(ToggleVector tv) {
    int i, count;

    count = 0;
    LOOP(count += oneSetBits(tv.cell[i]), i);

    return count;
}

inline static int64_t TVEC_ACTIVE_THREADS(ToggleVector tv) {
#if N_THREADS > 63
    int i;
    int64_t r;

    r = 0;
    LOOP(r |= tv.cell[i], i);
    return r;
#else
    return tv.cell[0];
#endif
}

#endif


