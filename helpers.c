/*
 *   Authors:
 *    Alexander Aring		<alex.aring@gmail.com>
 *
 *   This software is Copyright 2019 by the above mentioned author(s),
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <alex.aring@gmail.com>.
 */

#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>

#include "helpers.h"
#include "config.h"
#include "log.h"

int gen_stateless_addr(const struct in6_prefix *prefix,
		       const struct iface_llinfo *llinfo,
		       struct in6_addr *dst)
{
	uint8_t len = bits_to_bytes(prefix->len);
	int i;

	memset(dst, 0, sizeof(*dst));

	/* TODO only supported right now, gets tricky with bluetooth */
	if (prefix->len != 64 || llinfo->addr_len != 8)
		return -1;

	memcpy(dst, &prefix->prefix, len);

	for (i = 0; i < llinfo->addr_len; i++)
		dst->s6_addr[len + i] = llinfo->addr[i];

	/* U/L */
	dst->s6_addr[8] ^= 0x02;

	return 0;
}

__attribute__((format(printf, 1, 2))) static char *strdupf(char const *format, ...)
{
	va_list va;
	va_start(va, format);
	char *strp = 0;
	int rc = vasprintf(&strp, format, va);
	if (rc == -1 || !strp) {
		flog(LOG_ERR, "vasprintf failed: %s", strerror(errno));
		exit(-1);
	}
	va_end(va);

	return strp;
}

/* note: also called from the root context */
int set_var(const char *var, uint32_t val)
{
	int retval = -1;
	FILE *fp = 0;

	if (access(var, F_OK) != 0)
		goto cleanup;

	fp = fopen(var, "w");
	if (!fp) {
		flog(LOG_ERR, "failed to set %s: %s", var, strerror(errno));
		goto cleanup;
	}

	if (0 > fprintf(fp, "%u", val)) {
		goto cleanup;
	}

	retval = 0;

cleanup:
	if (fp)
		fclose(fp);

	return retval;
}

int set_interface_var(const char *iface, const char *var, const char *name, uint32_t val)
{
	int retval = -1;
	FILE *fp = 0;
	char *spath = strdupf(var, iface);

	/* No path traversal */
	if (!iface[0] || !strcmp(iface, ".") || !strcmp(iface, "..") || strchr(iface, '/'))
		goto cleanup;

	if (access(spath, F_OK) != 0)
		goto cleanup;

	fp = fopen(spath, "w");
	if (!fp) {
		if (name)
			flog(LOG_ERR, "failed to set %s (%u) for %s: %s", name, val, iface, strerror(errno));
		goto cleanup;
	}

	if (0 > fprintf(fp, "%u", val)) {
		goto cleanup;
	}

	retval = 0;

cleanup:
	if (fp)
		fclose(fp);

	free(spath);

	return retval;
}

void init_random_gen()
{
	srandom(time(NULL));
}

int gen_random_private_ula_pfx(struct in6_prefix *prefix)
{
	int i;

	prefix->len = 64;
	memset(&prefix->prefix, 0, sizeof(prefix->prefix));
	prefix->prefix.s6_addr[0] = 0xfd;
	for (i = 1; i < 6; i++)
		prefix->prefix.s6_addr[i] = random() & 0xff;

	return 0;
}
