/***************************************************************************
 *            format.h
 *
 *  Mon January 06 11:40:49 2014
 *  Copyright  2014  Dino Leonardo Sangoi
 *  <user@host>
 ****************************************************************************/
/*
 * format.h
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

#include <stdio.h>
#include <stdlib.h>

// pointer to a funtion that returns the value for a name.
// the function returns the size needed for the output.
typedef int (*formatResolver)(const char *name, char *output, size_t size, void *resolverData);

int vsnformat(char *output, size_t size, const char *format, formatResolver resolver, void *resolverData);
void testFormat(FILE *stream);
