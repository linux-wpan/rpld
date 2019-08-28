/*
 *   Authors:
 *    Alexander Aring		<alex.aring@gmail.com>
 *
 *   Original Authors:
 *    Lars Fenneberg		<lf@elemental.net>
 *
 *   This software is Copyright 1996,1997,2019 by the above mentioned author(s),
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <alex.aring@gmail.com>.
 */

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "buffer.h"
#include "log.h"

#define SAFE_BUFFER_INIT                                                                                                         \
	(struct safe_buffer) { .should_free = 0, .allocated = 0, .used = 0, .buffer = 0 }

struct safe_buffer *safe_buffer_new(void)
{
	struct safe_buffer *sb = malloc(sizeof(struct safe_buffer));
	*sb = SAFE_BUFFER_INIT;
	sb->should_free = 1;
	return sb;
}

void safe_buffer_free(struct safe_buffer *sb)
{
	if (sb->buffer) {
		free(sb->buffer);
	}

	if (sb->should_free) {
		free(sb);
	}
}

/**
 * Requests that the safe_buffer capacity be least n bytes in size.
 *
 * If n is greater than the current capacity, the function causes the container
 * to reallocate its storage increasing its capacity to n (or greater).
 *
 * In all other cases, the function call does not cause a reallocation and the
 * capacity is not affected.
 *
 * @param sb safe_buffer to enlarge
 * @param b Minimum capacity for the safe_buffer.
 */
static void safe_buffer_resize(struct safe_buffer *sb, size_t n)
{
	const int blocksize = 1 << 6; // MUST BE POWER OF 2.
	if (sb->allocated < n) {
		if (n % blocksize > 0) {
			n |= (blocksize - 1); // Set all the low bits
			n++;
		}
		if (n > 64 * 1024) {
			flog(LOG_ERR, "Requested buffer too large for any possible IPv6 ND, even with jumbogram.  Exiting.");
			exit(1);
		}
		sb->allocated = n;
		sb->buffer = realloc(sb->buffer, sb->allocated);
	}
}

size_t safe_buffer_pad(struct safe_buffer *sb, size_t count)
{
	safe_buffer_resize(sb, sb->used + count);
	memset(&sb->buffer[sb->used], 0, count);
	sb->used += count;
	return count;
}

size_t safe_buffer_append(struct safe_buffer *sb, const void *v, size_t count)
{
	if (sb) {
		unsigned const char *m = (unsigned const char *)v;
		safe_buffer_resize(sb, sb->used + count);
		memcpy(&sb->buffer[sb->used], m, count);
		sb->used += count;
	}

	return count;
}
