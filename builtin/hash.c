#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "builtin.h"

/* 
Hashing of primitive types as in 

https://github.com/python/cpython/blob/master/Python/pyhash.c

One of the design criteria for the Python hash algorithms is that e.g. 15 and 15.0 should hash to the same value.
This is not relevant in the typed context of Acton, but we still stick to this algorithm.

In Python, hash values may be negative, but -1 is used to signal an error. (When can hash computation fail in Acton?)
However, CPython uses unsigned type size_t for hash values.


*/

#ifdef __APPLE__
#  include <libkern/OSByteOrder.h>
#elif defined(HAVE_LE64TOH) && defined(HAVE_ENDIAN_H)
#  include <endian.h>
#elif defined(HAVE_LE64TOH) && defined(HAVE_SYS_ENDIAN_H)
#  include <sys/endian.h>
#endif

#define _PyHASH_BITS 61
#define _PyHASH_MODULUS (((long)1 << _PyHASH_BITS) - 1)
#define _PyHASH_INF 314159
#define _PyHASH_NAN 0
#define _PyHASH_MULTIPLIER 1000003UL  /* 0xf4243 */
#define SIZEOF_PY_UHASH_T 8

/* 
BvS 191002: For the moment, stick to little endian. In CPython, this is set in pyport.h based on info from the configure script.
*/

#define PY_BIG_ENDIAN 0
#define PY_LITTLE_ENDIAN 1

static long long_hash (long u) {
  long sign=1;
  if (u<0)  {

    sign=-1;
    u = -u;
  }
  long h = u % _PyHASH_MODULUS * sign;
  if (h == (long)-1)
    h = (long)-2;
  return h;
}

long $int_hash ($int n) {
  return long_hash(from$int(n));
}

static long double_hash(double d) {
    int e, sign;
    double m;
    long x, y;
    
    if (!isfinite(d)) {
        if (isinf(d))
            return d > 0 ? _PyHASH_INF : -_PyHASH_INF;
        else
            return _PyHASH_NAN;
    }
    
    m = frexp(d, &e);

    sign = 1;
    if (m < 0) {
        sign = -1;
        m = -m;
    }

    /* process 28 bits at a time;  this should work well both for binary
       and hexadecimal floating point. */
    x = 0;
    while (m) {
        x = ((x << 28) & _PyHASH_MODULUS) | x >> (_PyHASH_BITS - 28);
        m *= 268435456.0;  /* 2**28 */
        e -= 28;
        y = (long)m;  /* pull out integer part */
        m -= y;
        x += y;
        if (x >= _PyHASH_MODULUS)
            x -= _PyHASH_MODULUS;
    }

    /* adjust for the exponent;  first reduce it modulo _PyHASH_BITS */
    e = e >= 0 ? e % _PyHASH_BITS : _PyHASH_BITS-1-((-1-e) % _PyHASH_BITS);
    x = ((x << e) & _PyHASH_MODULUS) | x >> (_PyHASH_BITS - e);

    x = x * sign;
    if (x == (long)-1)
        x = (long)-2;
    return x;
}

long $float_hash($float v) {
  return double_hash(from$float(v));
}

long $pointer_hash($WORD p) {
    long x;
    long y = (long)p;
    // bottom 3 or 4 bits are likely to be 0; rotate y by 4 to avoid
    //  excessive hash collisions for dicts and sets 
    y = (y >> 4) | (y << 60);
    x = (long)y;
    if (x == -1)
        x = -2;
    return x;
}

/* hash secret
 *
 * memory layout on 64 bit systems
 *   cccccccc cccccccc cccccccc  uc -- unsigned char[24]
 *   pppppppp ssssssss ........  fnv -- two Py_hash_t
 *   k0k0k0k0 k1k1k1k1 ........  siphash -- two uint64_t
 *   ........ ........ ssssssss  djbx33a -- 16 bytes padding + one Py_hash_t
 *   ........ ........ eeeeeeee  pyexpat XML hash salt
 */


typedef union {
    // ensure 24 bytes 
    unsigned char uc[24];
    // two uint64 for SipHash24 
    struct {
        uint64_t k0;
        uint64_t k1;
    } siphash;
    struct {
        unsigned char padding[16];
        long hashsalt;
    } expat;
} _Py_HashSecret_t;


_Py_HashSecret_t _Py_HashSecret = {{0}};

#include "csiphash.c"

static long pysiphash(void *src, long src_sz) {
    return (long)siphash24(
        _le64toh(_Py_HashSecret.siphash.k0), _le64toh(_Py_HashSecret.siphash.k1),
        src, src_sz);
}

long $string_hash(void *src, int len) {
    long x;
    /*
      We make the hash of the empty string be 0, rather than using
      (prefix ^ suffix), since this slightly obfuscates the hash secret
    */
    if (len == 0) {
        return 0;
    }
         x = pysiphash(src, len);

    if (x == -1)
        return -2;
    return x;
}

   
/*
 "Old" hash algorithm for tuples; used in Python versions <= 3.7. 
    From 3.8 the xxHash-based algorithm above is used.
*/
long $tuple_hash($Hashable$tuple wit,$tuple tup) {
  int size = tup->size;
  long x = 0x345678UL;
  long y;
  long mult = _PyHASH_MULTIPLIER;
  for (int i=0; i < size; i++) {
    $Hashable h = wit->w$Hashable$tuple[i];
    y = from$int(h->$class->__hash__(h,tup->components[i]));
    x = (x ^ y) * mult;
    mult += (long)(82520UL + 2*(size-i-1));
  }
  x += 97531UL;
  if (x == (long)-1) 
    x = -2;
  return x;
}
