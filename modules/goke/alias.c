/**
 * @file alias.c  GOKE function alias
 *
 * Copyright (C) 2024 Florian Seybold
 */

#include <stdio.h>

int __fgetc_unlocked(FILE *stream) {
   return fgetc_unlocked(stream);
}
size_t _stdlib_mb_cur_max() {
   return __ctype_get_mb_cur_max();
}
const unsigned short * * __ctype_b() {
   return __ctype_b_loc();
}
