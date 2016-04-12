/*
 * gaim - Blogger (xml-rpc) Protocol Plugin
 *
 * Copyright (C) 2003, Herman Bloggs <hermanator12002@yahoo.com>
 * GPL Software Supported by www.Madster.com
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef _STACK_H_
#define _STACK_H_

typedef struct _stack_item {
	void *data;
	struct _stack_item *next;
} stack_item;

typedef struct _stack {
	struct _stack_item *top;
} stack;

stack *stack_new();
void stack_push( stack *s, void *data );
void *stack_pop( stack *s );
void *stack_top( stack *s );
int stack_empty(stack *s);
void stack_free(stack *s);

#endif /* _STACK_H_ */
