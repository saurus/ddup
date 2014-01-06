/***************************************************************************
 *            format.c
 *
 *  Mon January 06 11:40:49 2014
 *  Copyright  2014  Dino Leonardo Sangoi
 *  <user@host>
 ****************************************************************************/
/*
 * format.c
 *
 * Copyright (C) 2014 - Dino Leonardo Sangoi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// for strndup
#define _POSIX_C_SOURCE 200809L

#include "format.h"
#include <stdio.h>
#include <string.h>

int vsnformat(char *output, size_t size, const char *format, formatResolver resolver, void *resolverData)
{
	char *d = output;
	size_t l = size;
	const char *f = format;
	char *name;
	
	do {
		size_t len = strcspn(f, "%");
		if (l <= len)
			return -1;
		
		memcpy(d, f, len);
		d += len;
		f += len;
		l -= len;
		
		if (*f == '\0')
			break; // the end

		f++;
		if (*f == '%') {
			if (l <= 1)
				return -1;
			*d++ = '%';
			f++;
			l++;
		} else {
			size_t p = strcspn(f, ".");

			if (p == 0) {
				fprintf(stderr, "vsnformat: invalid format string (ends with '%%' or data name missing), at pos %zu in format string (%.80s)\n", (f - format), (f-1));
				return -2;
			}

			if (f[p] == '\0') {
				fprintf(stderr, "vsnformat: invalid format string (missing '.' after data name), at pos %zu in format string (%.80s)\n", (f - format), (f-1));
				return -2;
			}

			name = strndup(f, p);
			len = resolver(name, d, l, resolverData);
			free(name);

			if (len >= l)
				return -1;

			d += len;
			f += (p + 1);
			l -= len;
		}
	} while (*f != '\0'); 

	*d = '\0';
	

	return (d - output);         
}

int testResolver(const char *name, char *output, size_t size, void *dummy)
{
	size_t len = strlen(name);

	if (len + 2 >= size)
		return -1;

	*output++ = '#';
	strcpy(output, name);
	output += len;
	*output++ = '#';
	
	return len + 2;
}

void testFormat(FILE *stream) 
{
	char format[1024];
	char output[10240];
	int len;
	int res;

	while (fgets(format, 1024, stream) != NULL)
	{
		len = strlen(format);
		if (len == 0) {
			printf("empty format, skipping...\n");
			continue;
		}

		if (format[len - 1] != '\n') {
			printf("format too long, skipping...\n");
			continue;
		}

		printf("INPUT : %s", format);

		memset(output, 0, 10240);
		res = vsnformat(output, 10240, format, testResolver, NULL);

		printf("OUTPUT[%d]: %s\n", res, output);
	}
}