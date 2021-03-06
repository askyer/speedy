/* compute the Adler-32 checksum of a data buffer
 * Copyright (C) 1995-2004 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "spy_adler32.h"

/* @(#) $Id$ */
#define Z_NULL 0
#define ZEXPORT
#define ZLIB_INTERNAL
#define BASE 65521UL    /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf,i)  {adler += (buf)[i]; sum2 += adler;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

/* use NO_DIVIDE if your processor does not do division in hardware */
#ifdef NO_DIVIDE
#  define MOD(a) \
    do { \
        if (a >= (BASE << 16)) a -= (BASE << 16); \
        if (a >= (BASE << 15)) a -= (BASE << 15); \
        if (a >= (BASE << 14)) a -= (BASE << 14); \
        if (a >= (BASE << 13)) a -= (BASE << 13); \
        if (a >= (BASE << 12)) a -= (BASE << 12); \
        if (a >= (BASE << 11)) a -= (BASE << 11); \
        if (a >= (BASE << 10)) a -= (BASE << 10); \
        if (a >= (BASE << 9)) a -= (BASE << 9); \
        if (a >= (BASE << 8)) a -= (BASE << 8); \
        if (a >= (BASE << 7)) a -= (BASE << 7); \
        if (a >= (BASE << 6)) a -= (BASE << 6); \
        if (a >= (BASE << 5)) a -= (BASE << 5); \
        if (a >= (BASE << 4)) a -= (BASE << 4); \
        if (a >= (BASE << 3)) a -= (BASE << 3); \
        if (a >= (BASE << 2)) a -= (BASE << 2); \
        if (a >= (BASE << 1)) a -= (BASE << 1); \
        if (a >= BASE) a -= BASE; \
    } while (0)
#  define MOD4(a) \
    do { \
        if (a >= (BASE << 4)) a -= (BASE << 4); \
        if (a >= (BASE << 3)) a -= (BASE << 3); \
        if (a >= (BASE << 2)) a -= (BASE << 2); \
        if (a >= (BASE << 1)) a -= (BASE << 1); \
        if (a >= BASE) a -= BASE; \
    } while (0)
#else
#  define MOD(a) a %= BASE
#  define MOD4(a) a %= BASE
#endif

/*
uint64_t spy_buffer_adler32(uint64_t adler, spy_rw_buffer_t *buffer, size_t len)
{
	char *data;
	size_t data_len, i, nread, left, read_pos = buffer->read_pos;
	uint64_t sum2;

	sum2 = (adler >> 16) & 0xffff;
	adler &= 0xffff;

	assert (buffer->write_pos - buffer->read_pos >= len);

	left = len;
	while (left > 0) {
		assert (spy_rw_buffer_next_readable(buffer, (char**)(&data), &data_len) == 0); 

		nread = data_len > left ? left : data_len;

		for (i = 0; i < nread; i++) {
			adler = (adler + data[i]) % BASE;
			sum2 = (sum2 + adler) % BASE;
		}

		left -= nread;
		buffer->read_pos += nread;
	}

	spy_log(ERROR, "buffer adler32. adler:%lu, sum2:%lu\n", adler, sum2);

	spy_rw_buffer_reset_read(buffer, read_pos);

	return (sum2 << 16) | adler;
}

uint64_t spy_simple_adler32(uint64_t adler, uint8_t *data, size_t len)
{
	size_t i;

	uint64_t sum2 = (adler >> 16) & 0xffff;
	adler &= adler & 0xffff;

	for (i = 0; i < len; i++) {
		adler = (adler + data[i]) % BASE;
		sum2 = (sum2 + adler) % BASE;
	}

	return (sum2 << 16) | adler;
}
*/

uint64_t spy_buffer_adler32(uint64_t adler, spy_rw_buffer_t *buffer, size_t len)
{
	uint8_t *data;
	size_t data_len, n, nread, left, read_pos = buffer->read_pos;
	uint64_t sum2;

	assert (buffer->write_pos - buffer->read_pos >= len);

	sum2 = (adler >> 16) & 0xffff;
	adler &= 0xffff;

	left = len;
	while (left > 0) {
		assert (spy_rw_buffer_next_readable(buffer, (char**)(&data), &data_len) == 0); 

		nread = data_len > left ? left : data_len;
		data_len = nread;

		while (data_len >= NMAX) {
			data_len -= NMAX;
			n = NMAX / 16;          /* NMAX is divisible by 16 */
			do {
				DO16(data);          /* 16 sums unrolled */
				data += 16;
			} while (--n);
			adler %= BASE;
			sum2 %= BASE;
		}

	    /* do remaining bytes (less than NMAX, still just one modulo) */
		if (data_len) {                  /* avoid modulos if none remaining */
			while (data_len >= 16) {
				data_len -= 16;
				DO16(data);
				data += 16;
			}
			while (data_len--) {
				adler += *data++;
				sum2 += adler;
			}
			adler %= BASE;
			sum2 %= BASE;
		}

		left -= nread;
		buffer->read_pos += nread;
	} // end of while (left > 0)

	spy_rw_buffer_reset_read(buffer, read_pos);

	return adler | (sum2 << 16);
}

/* ========================================================================= */
uLong ZEXPORT spy_adler32(adler, buf, len)
    uLong adler;
    const Byte *buf;
    uInt len;
{
    unsigned long sum2;
    unsigned n;

    /* split Adler-32 into component sums */
    sum2 = (adler >> 16) & 0xffff;
    adler &= 0xffff;

    /* in case user likes doing a byte at a time, keep it fast */
    if (len == 1) {
        adler += buf[0];
        if (adler >= BASE)
            adler -= BASE;
        sum2 += adler;
        if (sum2 >= BASE)
            sum2 -= BASE;
        return adler | (sum2 << 16);
    }

    /* initial Adler-32 value (deferred check for len == 1 speed) */
    if (buf == Z_NULL)
        return 1L;

    /* in case short lengths are provided, keep it somewhat fast */
    if (len < 16) {
        while (len--) {
            adler += *buf++;
            sum2 += adler;
        }
        if (adler >= BASE)
            adler -= BASE;
        MOD4(sum2);             /* only added so many BASE's */
        return adler | (sum2 << 16);
    }

    /* do length NMAX blocks -- requires just one modulo operation */
    while (len >= NMAX) {
        len -= NMAX;
        n = NMAX / 16;          /* NMAX is divisible by 16 */
        do {
            DO16(buf);          /* 16 sums unrolled */
            buf += 16;
        } while (--n);
        MOD(adler);
        MOD(sum2);
    }

    /* do remaining bytes (less than NMAX, still just one modulo) */
    if (len) {                  /* avoid modulos if none remaining */
        while (len >= 16) {
            len -= 16;
            DO16(buf);
            buf += 16;
        }
        while (len--) {
            adler += *buf++;
            sum2 += adler;
        }
        MOD(adler);
        MOD(sum2);
    }

    /* return recombined sums */
    return adler | (sum2 << 16);
}

/* ========================================================================= */
uLong ZEXPORT adler32_combine(adler1, adler2, len2)
    uLong adler1;
    uLong adler2;
    off_t len2;
{
    unsigned long sum1;
    unsigned long sum2;
    unsigned rem;

    /* the derivation of this formula is left as an exercise for the reader */
    rem = (unsigned)(len2 % BASE);
    sum1 = adler1 & 0xffff;
    sum2 = rem * sum1;
    MOD(sum2);
    sum1 += (adler2 & 0xffff) + BASE - 1;
    sum2 += ((adler1 >> 16) & 0xffff) + ((adler2 >> 16) & 0xffff) + BASE - rem;
    if (sum1 > BASE) sum1 -= BASE;
    if (sum1 > BASE) sum1 -= BASE;
    if (sum2 > (BASE << 1)) sum2 -= (BASE << 1);
    if (sum2 > BASE) sum2 -= BASE;
    return sum1 | (sum2 << 16);
}
