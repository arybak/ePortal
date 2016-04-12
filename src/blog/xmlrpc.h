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
#ifndef _XMLRPC_H_
#define _XMLRPC_H_

/* Allow for macro expansion of the args */

#define XMLRPC_RESPONSE(x)        ((struct xmlrpc_response*)x)
#define XMLRPC_PARAM(x)           ((struct xmlrpc_param*)x)
#define XMLRPC_FAULT(x)           ((struct xmlrpc_fault*)x)
#define XMLRPC_VALUE(x)           ((struct xmlrpc_value*)x)
#define XMLRPC_STRUCT(x)          ((struct xmlrpc_struct*)x)
#define XMLRPC_STRUCT_MEMBER(x)   ((struct xmlrpc_struct_member*)x)
#define XMLRPC_ARRAY(x)           ((struct xmlrpc_array*)x)
#define XMLRPC_DATA(x)            ((struct xmlrpc_data*)x)
#define XMLRPC_TAG(x)             ((struct xmlrpc_tag*)x)

#define join2p(a,b)               a->b
#define join3p(a,b,c)             a->b->c
#define join4p(a,b,c,d)           a->b->c->d
#define SAFE_POINTER_2(a,b)       (a ? (join2p(a,b) ? (join2p(a,b)) : NULL) : NULL)
#define SAFE_POINTER_3(a,b,c)     (a ? (join2p(a,b) ? (join3p(a,b,c) ? (join3p(a,b,c)) : NULL) : NULL) : NULL)
#define SAFE_POINTER_4(a,b,c,d)   (a ? (join2p(a,b) ? (join3p(a,b,c) ? (join4p(a,b,c,d) ? (join4p(a,b,c,d)) : NULL) : NULL) : NULL) : NULL)

typedef enum _response_type {
	valid,
	fault
} response_type;

typedef enum _xmlrpc_type {
	Integer_T,
	Boolean_T,
	String_T,
	Double_T,
	DateTime_iso8601_T,
	Base64_T,
	Struct_T,
	Array_T
} xmlrpc_type;

struct xmlrpc_value {
	xmlrpc_type type;
	void *data;
	int data_len; /* used for scalar types */
};

struct xmlrpc_param {
	struct xmlrpc_value *value;
};

struct xmlrpc_struct_member {
	char* name;
	struct xmlrpc_value *value;
};

struct xmlrpc_struct {
	struct xmlrpc_struct_member *member;
	struct xmlrpc_struct *next;
};

struct xmlrpc_array {
	struct xmlrpc_data *data;
};

struct xmlrpc_data {
	struct xmlrpc_value *value;
	struct xmlrpc_data *next;
};

struct xmlrpc_fault {
	struct xmlrpc_value *value;
};

struct xmlrpc_response {
	response_type type;
	void *data;
};

void xmlrpc_free_response(struct xmlrpc_response *resp);
struct xmlrpc_response *xmlrpc_parse(const char *xml_buffer);

#endif /* _XMLRPC_H_ */
