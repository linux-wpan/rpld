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

#ifndef __RPLD_BUFFER_H__
#define __RPLD_BUFFER_H__

struct safe_buffer *safe_buffer_new(void);
void safe_buffer_free(struct safe_buffer *sb);
size_t safe_buffer_pad(struct safe_buffer *sb, size_t count);
size_t safe_buffer_append(struct safe_buffer *sb, const void *v, size_t count);

struct safe_buffer {
	int should_free;
	size_t allocated;
	size_t used;
	unsigned char *buffer;
};

#endif /* __RPLD_BUFFER_H__ */
