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

/* automatic partial unroling*/
#if N_THREADS <= 0
#   error Unacceptable N_THREADS size
#elif N_THREADS <= _TVEC_BIWORD_SIZE_
#   define _TVEC_CELLS_               1
#   define LOOP(EXPR, I)              {I=0;EXPR;}
#elif N_THREADS <= 2 * _TVEC_BIWORD_SIZE_
#   define _TVEC_CELLS_               2
#   define LOOP(EXPR, I)              {I=0;EXPR; I++; EXPR;}
#elif N_THREADS <= 3 * _TVEC_BIWORD_SIZE_
#   define _TVEC_CELLS_               3
#   define LOOP(EXPR, I)              {I=0;EXPR; I++; EXPR; I++; EXPR;}
#else
#   define _TVEC_CELLS_               ((N_THREADS >> _TVEC_DIVISION_SHIFT_BITS_) + 1)
#   define LOOP(EXPR, I)              {for (I = 0; I < _TVEC_CELLS_; I++) {EXPR;}}
#endif

typedef struct ToggleVector {
    bitword_t cell[_TVEC_CELLS_];
} ToggleVector;


// Operations that handle banks of bits and no the whole vectors
// -------------------------------------------------------------

inline static int TVEC_GET_BANK_OF_BIT(int bit) {
#if N_THREADS > _TVEC_BIWORD_SIZE_
    return bit >> _TVEC_DIVISION_SHIFT_BITS_;
#else
    return 0;
#endif
}

inline static void TVEC_ATOMIC_COPY_BANKS(volatile ToggleVector *tv1, volatile ToggleVector *tv2, int bank) {
    tv1->cell[bank] = tv2->cell[bank];
}

inline static void TVEC_ATOMIC_ADD_BANK(volatile ToggleVector *tv1, ToggleVector *tv2, int bank) {
#if _TVEC_BIWORD_SIZE_ == 32
    FAA32(&tv1->cell[bank], tv2->cell[bank]);
#else
    FAA64(&tv1->cell[bank], tv2->cell[bank]);
#endif
}

inline static void TVEC_NEGATIVE_BANK(ToggleVector *tv1, ToggleVector *tv2, int bank) {
    tv1->cell[bank] = -tv2->cell[bank];
}

inline static void TVEC_XOR_BANKS(ToggleVector *res, ToggleVector *tv1, ToggleVector *tv2, int bank) {
    res->cell[bank] = tv1->cell[bank] ^ tv2->cell[bank];
}

inline static void TVEC_AND_BANKS(ToggleVector *res, ToggleVector *tv1, ToggleVector *tv2, int bank) {
    res->cell[bank] = tv1->cell[bank] & tv2->cell[bank];
}

// Oprations that handle the whole vectors of bits
// -----------------------------------------------

inline static ToggleVector TVEC_NEGATIVE(ToggleVector tv) {
    int i = 0;
    ToggleVector res;

    LOOP(res.cell[i] = -tv.cell[i], i);
    return res;
}


inline static void TVEC_REVERSE_BIT(ToggleVector *tv1, int bit) {
#if N_THREADS > _TVEC_BIWORD_SIZE_
    int i, offset;

    i = bit >> _TVEC_DIVISION_SHIFT_BITS_;
    offset = bit & _TVEC_MODULO_BITS_;
    tv1->cell[i] ^= 1L << offset;
#else
    tv1->cell[0] ^= 1L << bit;
#endif
}


inline static void TVEC_SET_BIT(ToggleVector *tv1, int bit) {
#if N_THREADS > _TVEC_BIWORD_SIZE_
    int i, offset;

    i = bit >> _TVEC_DIVISION_SHIFT_BITS_;
    offset = bit & _TVEC_MODULO_BITS_;
    tv1->cell[i] |= 1L << offset;
#else
    tv1->cell[0] |= 1L << bit;
#endif
}


inline static void TVEC_SET_ZERO(ToggleVector *tv1) {
    int i;

    LOOP(tv1->cell[i] = 0L, i);
}


inline static bool TVEC_IS_SET(ToggleVector tv1, int pid) {
#if N_THREADS > _TVEC_BIWORD_SIZE_
    int i, offset;

    i = pid >> _TVEC_DIVISION_SHIFT_BITS_;
    offset = pid & _TVEC_MODULO_BITS_;
    // The commented code was optimized in order to avoid branches
    // if ( (tv1.cell[i] & (1L << offset)) ==  0) return false;
    // else return true;
    return (tv1.cell[i] >> offset) & 1;
#else
    // The commented code was optimized in order to avoid branches
    // if ( (tv1.cell[i] & (1L << offset)) ==  0) return false;
    // else return true;
    return (tv1.cell[0] >> pid) & 1;
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

inline static int TVEC_COUNT_BITS(ToggleVector tv) {
    int i, count;

    count = 0;
    LOOP(count += oneSetBits(tv.cell[i]), i);

    return count;
}

#endif


