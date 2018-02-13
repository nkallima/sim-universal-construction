#ifndef _TYPES_H_
#define _TYPES_H_

#include <system.h>
#include <stdint.h>

typedef union int_aligned32_t {
   int32_t v CACHE_ALIGN;
   char pad[CACHE_LINE_SIZE];
}  int_aligned32_t;

typedef union int_aligned64_t {
   int64_t v CACHE_ALIGN;
   char pad[CACHE_LINE_SIZE];
}  int_aligned64_t;

typedef union ptr_aligned_t {
   void *ptr CACHE_ALIGN;
   char pad[CACHE_LINE_SIZE];
}  ptr_aligned_t;

#define null                           NULL
#define bool                           int32_t
#define true                           1
#define false                          0

#endif
