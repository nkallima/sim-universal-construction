/// @file backoff.h
/// @brief This file exposes the API of a simple backoff implementation for lock-free and wait-free data-structures.
/// This backoff scheme is based on the backoff implementation proposed by Maged M. Michael, and Michael L. Scott. 
/// in "Simple, fast, and practical non-blocking and blocking concurrent queue algorithms",
/// Proceedings of the fifteenth annual ACM symposium on Principles of distributed computing, 1996.
/// Examples of usage could be found in the lock-free stack and queue implementations 
/// (i.e. libconcurrent/concurrent/lfstack.c and libconcurrent/concurrent/msqueue.c respectively).
#ifndef _BACKOFF_H_
#define _BACKOFF_H_

/// @brief BackoffStruct stores the state of an instance of the a backoff object.
/// BackoffStruct should be initialized using the init_backoff function.
typedef struct BackoffStruct {
    /// @brief The current value of the backoff scheme.
    unsigned backoff;
    /// @brief The initial value of the backoff scheme is 2^backoff_base_bits.
    unsigned backoff_base_bits;
    /// @brief The maximum value of the backoff scheme is 2^backoff_cap_bits.
    unsigned backoff_cap_bits;
    /// @brief The step of the backoff scheme is 2^backoff_shift_bits - 1.
    unsigned backoff_shift_bits;
    /// @brief The minimum value of the backoff scheme (equal to 2^backoff_base_bits).
    unsigned backoff_base;
    /// @brief The maximum value of the backoff scheme (equal to 2^backoff_cap_bits).
    unsigned backoff_cap;
    /// @brief The step of the backoff scheme is 2^backoff_shift_bits - 1.
    unsigned backoff_addend;
} BackoffStruct;

/// @brief This function initializes an instance of a backoff object. 
/// Each threads should use a different instance of this object for each concurrent data-structure that it accesses.
/// This function should be called by each thread that is going to access.
///
/// @param b A pointer to an instance of the backoff object.
/// @param base_bits The initial value of the backoff scheme is 2^base_bits.
/// @param cap_bits The maximum value of the backoff scheme is 2^cap_bits.
/// @param shift_bits The step of the backoff scheme is 2^shift_bits - 1.
void init_backoff(BackoffStruct *b, unsigned base_bits, unsigned cap_bits, unsigned shift_bits);

/// @brief This function resets the current value of the backoff scheme to 2^base_bits.
///
/// @param b A pointer to an instance of the backoff object.
void reset_backoff(BackoffStruct *b);

/// @brief This function applies backoff.
///
/// @param b A pointer to an instance of the backoff object.
void backoff_delay(BackoffStruct *b);

/// @brief This function reduces the current backoff value by 2^shift_bits only in case that the 
/// current value is greater than 2^base_bits.
///
/// @param b A pointer to an instance of the backoff object.
void backoff_reduce(BackoffStruct *b);

/// @brief This function increases the current backoff value by 2^shift_bits only in case that the 
/// current value is not greater than 2^caps_bits.
///
/// @param b A pointer to an instance of the backoff object.
void backoff_increase(BackoffStruct *b);

#endif
