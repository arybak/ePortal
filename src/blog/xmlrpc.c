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
#include <stdio.h>
#include <string.h>
#include <glib.h>
#ifndef _WIN32
#include <expat.h>
#else
#include "xmlparse.h"
#endif
#include "stack.h"
#include "xmlrpc.h"
#include "debug.h"

/* 1 - Debugging On  0 - Debugging Off */
#define XMLRPC_DEBUG 0

#if XMLRPC_DEBUG
#define xmlrpc_debug(msg, args...) gaim_debug(GAIM_DEBUG_INFO, "blogger", "xmlrpc: " ## msg ##, ## args)
#else
#define xmlrpc_debug(msg, args...)
#endif

typedef enum _parse_state {
	MethodResponse = 0,
	Params,
	Param,
	Value,
	Integer,
	Boolean,
	String,
	Double,
	DateTime_iso8601,
	Base64,
	Struct,
	Member,
	Name,
	Array,
	Data,
	Fault,
	Unknown
} parse_state;

typedef enum _parse_event {
	start_element,
	end_element,
	char_element
} parse_event;

struct state_data {
	int error;
	struct _stack *tag_stack;
	struct xmlrpc_response *response;
};

struct xmlrpc_tag {
	parse_state name;
	void *data;
};


/*
 *  PRIVATE CODE
 */

static void xmlrpc_parse_error(struct state_data *sd, const char *fmt, ...) {
	va_list ap;
	gchar *s;

	va_start(ap, fmt);
	s = g_strdup_vprintf(fmt, ap);
	va_end(ap);

	sd->error = 1;
	xmlrpc_debug("xmlrpc parse error: %s\n", s);
	g_free(s);
}

static parse_state get_parse_state(const char* tag) {
	parse_state ret;

	if(strcmp("methodResponse", tag)==0)
		ret = MethodResponse;
	else if(strcmp("params", tag)==0)
		ret = Params;
	else if(strcmp("param", tag)==0)
		ret = Param;
	else if(strcmp("value", tag)==0)
		ret = Value;
	else if(strcmp("int", tag)==0 ||
		strcmp("i4", tag)==0)
		ret = Integer;
	else if(strcmp("boolean", tag)==0)
		ret = Boolean;
	else if(strcmp("string", tag)==0)
		ret = String;
	else if(strcmp("double", tag)==0)
		ret = Double;
	else if(strcmp("dateTime.iso8601", tag)==0)
		ret = DateTime_iso8601;
	else if(strcmp("base64", tag)==0)
		ret = Base64;
	else if(strcmp("struct", tag)==0)
		ret = Struct;
	else if(strcmp("member", tag)==0)
		ret = Member;
	else if(strcmp("name", tag)==0)
		ret = Name;
	else if(strcmp("array", tag)==0)
		ret = Array;
	else if(strcmp("data", tag)==0)
		ret = Data;
	else if(strcmp("fault", tag)==0)
		ret = Fault;
	else
		ret = Unknown;

	return ret;
}

static struct xmlrpc_tag *new_tag(parse_state name) {
	struct xmlrpc_tag *tag = g_new0(struct xmlrpc_tag, 1);

	if(!tag)
		return NULL;

	tag->name = name;

	switch(name) {
	case Unknown: {
		g_free(tag);
		return NULL;
	}
	case Param:
		tag->data = g_new0(struct xmlrpc_param, 1);
		break;
	case Value:
		tag->data = g_new0(struct xmlrpc_value, 1);
		break;
	case Member:
		tag->data = g_new0(struct xmlrpc_struct_member, 1);
		break;
	case MethodResponse:
		tag->data = g_new0(struct xmlrpc_response, 1);
		break;
	case Array:
		tag->data = g_new0(struct xmlrpc_array, 1);
		break;
	case Data:
		tag->data = g_new0(struct xmlrpc_data, 1);
		break;
	case Struct:
		tag->data = g_new0(struct xmlrpc_struct, 1);
		break;
	case Fault:
		tag->data = g_new0(struct xmlrpc_fault, 1);
		break;
	case Params:
	case Name:
	case Integer:
	case Boolean:
	case String:
	case Double:
	case DateTime_iso8601:
	case Base64:
	default:
		/* not data */
		tag->data = NULL;
	}

	return tag;
}

#define CHECK_POINTER( pointer ) \
{ \
	if(!pointer) { \
		xmlrpc_parse_error(sd, "Received unexpected NULL pointer\n"); \
		return; \
	} \
}

#define CHECK_TAG( tag, value ) \
{ \
	if(tag != value) { \
		xmlrpc_parse_error(sd, "Expected <%s> prior to current tag\n", value); \
		return; \
	} \
}

static void state_machine(parse_event event, void *userdata, const char *name, int namelen,  const char **atts) {
	struct state_data *sd = (struct state_data*)userdata;
	struct xmlrpc_tag *tag=NULL;
	parse_state state;

	/* Once an error has occured, stop parsing */
	if(sd->error)
		return;
	
	/* convert tag string to enum type */
	if(event == start_element ||
	   event == end_element)
		state = get_parse_state(name);

	switch(event) {
	case start_element:
		xmlrpc_debug("Open tag: %s\n", name);

		switch(state) {
		case MethodResponse: {
			tag = new_tag(MethodResponse);
			CHECK_POINTER(tag);
			sd->response = XMLRPC_RESPONSE(tag->data);
			stack_push(sd->tag_stack, tag);
			CHECK_POINTER(sd->tag_stack);
			break;
		}
		case Fault: {
			tag = XMLRPC_TAG(stack_top(sd->tag_stack));
			CHECK_POINTER(tag);
			CHECK_TAG(tag->name, MethodResponse);
			CHECK_POINTER(sd->response);
			sd->response->type = fault;
			tag = new_tag(Fault);
			CHECK_POINTER(tag);
			sd->response->data = tag->data;
			stack_push(sd->tag_stack, tag);
			break;
		}
		case Params: {
			tag = XMLRPC_TAG(stack_top(sd->tag_stack));
			CHECK_POINTER(tag);
			CHECK_TAG(tag->name, MethodResponse);
			CHECK_POINTER(sd->response);
			sd->response->type = valid;
			tag = new_tag(Params);
			CHECK_POINTER(tag);
			stack_push(sd->tag_stack, tag);
			CHECK_POINTER(sd->tag_stack);
			break;
		}
		case Param: {
			tag = XMLRPC_TAG(stack_top(sd->tag_stack));
			CHECK_POINTER(tag);
			CHECK_TAG(tag->name, Params);
			CHECK_POINTER(sd->response);
			/* params only allowed one param */
			if(sd->response->data) {
				xmlrpc_parse_error(sd, "<params> may only contain one <param> tag\n");
				return;
			}
			tag = new_tag(Param);
			CHECK_POINTER(tag);
			sd->response->data = tag->data;
			stack_push(sd->tag_stack, tag);
			break;
		}
		case Value: {
			tag = XMLRPC_TAG(stack_top(sd->tag_stack));
			CHECK_POINTER(tag);

			if(tag->name == Param) {
				struct xmlrpc_param *param;
				
				CHECK_POINTER(tag->data);
				param = XMLRPC_PARAM(tag->data);
				/* We're only allowed one param value */
				if(param->value) {
					xmlrpc_parse_error(sd, "<param> may only contain one <value> tag\n");
					return;
				}
				tag = new_tag(Value);
				CHECK_POINTER(tag);
				param->value = XMLRPC_VALUE(tag->data);
				stack_push(sd->tag_stack, tag);
			}
			else if(tag->name == Data) {
				struct xmlrpc_data *data;

				CHECK_POINTER(tag->data);
				data = XMLRPC_DATA(tag->data);
				/* Go down data list */
				if(data->value) {
					while(data->next)
						data = data->next;
					data->next = g_new0(struct xmlrpc_data, 1);
					CHECK_POINTER(data->next);
					data = data->next;
				}
				tag = new_tag(Value);
				CHECK_POINTER(tag);
				data->value = XMLRPC_VALUE(tag->data);
				stack_push(sd->tag_stack, tag);
			}
			else if(tag->name == Member) {
				struct xmlrpc_struct_member *member;

				CHECK_POINTER(tag->data);
				member = XMLRPC_STRUCT_MEMBER(tag->data);
				tag = new_tag(Value);
				CHECK_POINTER(tag);
				member->value = XMLRPC_VALUE(tag->data);
				stack_push(sd->tag_stack, tag);
			}
			else if(tag->name == Fault) {
				struct xmlrpc_fault *fault;
				CHECK_POINTER(tag->data);
				fault = XMLRPC_FAULT(tag->data);
				tag = new_tag(Value);
				CHECK_POINTER(tag);
				fault->value = XMLRPC_VALUE(tag->data);
				stack_push(sd->tag_stack, tag);
			}
			else {
				xmlrpc_parse_error(sd, "Expected <param>, <data>, <member> or <fault> prior to <value> tag\n");
				return;
			}
			break;
		}
		case Data: {
			tag = XMLRPC_TAG(stack_top(sd->tag_stack));
			CHECK_POINTER(tag);

			if(tag->name == Array) {
				struct xmlrpc_array *array;

				CHECK_POINTER(tag->data);
				array = XMLRPC_ARRAY(tag->data);
				tag = new_tag(Data);
				CHECK_POINTER(tag);
				array->data = XMLRPC_DATA(tag->data);
			}
			stack_push(sd->tag_stack, tag);
			break;
		}
		case Array:
		case Struct: {
			struct xmlrpc_value *value;

			tag = XMLRPC_TAG(stack_top(sd->tag_stack));
			CHECK_POINTER(tag);
			CHECK_TAG(tag->name, Value);
			CHECK_POINTER(tag->data);
			value = XMLRPC_VALUE(tag->data);
			tag = new_tag(state);
			CHECK_POINTER(tag);
			/* Make sure to free char data that may have been set
			   after the value tag. This will happen because a
			   value without specific type tags defaults to the
			   string type */
			if(value->data && (value->type == String_T)) {
				xmlrpc_debug("Free bogus char element\n");
				g_free(value->data);
				value->data_len = 0;
			}
			value->type = (state == Array) ? Array_T : Struct_T;
			value->data = tag->data;
			stack_push(sd->tag_stack, tag);
			break;
		}
		case Member: {
			struct xmlrpc_struct *sTruct;

			tag = XMLRPC_TAG(stack_top(sd->tag_stack));
			CHECK_POINTER(tag);
			CHECK_TAG(tag->name, Struct);
			CHECK_POINTER(tag->data);
			sTruct = XMLRPC_STRUCT(tag->data);
			/* Go down struct member list */
			if(sTruct->member) {
				while(sTruct->next)
					sTruct = sTruct->next;
				sTruct->next = g_new0(struct xmlrpc_struct, 1);
				CHECK_POINTER(sTruct->next);
				sTruct = sTruct->next;
			}
			tag = new_tag(Member);
			CHECK_POINTER(tag);
			sTruct->member = XMLRPC_STRUCT_MEMBER(tag->data);
			stack_push(sd->tag_stack, tag);
			break;
		}
		case Name: {
			void *member;
			tag = XMLRPC_TAG(stack_top(sd->tag_stack));
			CHECK_POINTER(tag);
			CHECK_TAG(tag->name, Member);
			CHECK_POINTER(tag->data);
			member = tag->data;
			tag = new_tag(Name);
			CHECK_POINTER(tag);
			/* Pass member struct to name tag */
			tag->data = member;
			stack_push(sd->tag_stack, tag);
			break;
		}
		/* Simple value types */
		case Boolean:
		case String:
		case Double:
		case Integer:
		case DateTime_iso8601:
		case Base64:
		{
			struct xmlrpc_value *value;

			tag = XMLRPC_TAG(stack_top(sd->tag_stack));
			CHECK_POINTER(tag);
			CHECK_TAG(tag->name, Value);
			value = XMLRPC_VALUE(tag->data);
			tag = new_tag(state);
			/* Make sure to free char data that may have been set
			   after the value tag. This will happen because a
			   value without specific type tags defaults to the
			   string type */
			if(value->data && (value->type == String_T)) {
				xmlrpc_debug("Free bogus char element\n");
				g_free(value->data);
				value->data_len = 0;
			}
			/* Pass value struct to simple value type tag */
			tag->data = (void*)value;
			stack_push(sd->tag_stack, tag);
			break;
		}
		case Unknown:
		default:
		{
			xmlrpc_parse_error(sd, "Unknown xmlrpc open tag: %s\n", name);
			return;
		}
		}/*end switch*/
		break;
	case end_element:
		xmlrpc_debug("Close tag: %s\n", name);

		tag = XMLRPC_TAG(stack_pop(sd->tag_stack));
		CHECK_POINTER(tag);

		if(tag->name != state) {
			/* We're not likely to get here since the xml parser should
			   catch this first. */
			xmlrpc_parse_error(sd, "Close tag %s, does not match the open tag on the stack\n", name);
			return;
		}
		g_free(tag);
		break;
	case char_element:
		tag = XMLRPC_TAG(stack_top(sd->tag_stack));
		CHECK_POINTER(tag);
		switch(tag->name) {
		/* Simple value types */
		case Boolean:
		case String:
		case Double:
		case Integer:
		case DateTime_iso8601:
		case Base64: 
		case Value: {
			struct xmlrpc_value *value;

			CHECK_POINTER(tag->data);
			value = XMLRPC_VALUE(tag->data);
			/* set value type */
			if(tag->name == Boolean)
				value->type = Boolean_T;
			else if(tag->name == String)
				value->type = String_T;
			else if(tag->name == Double)
				value->type = Double_T;
			else if(tag->name == Integer)
				value->type = Integer_T;
			else if(tag->name == DateTime_iso8601)
				value->type = DateTime_iso8601_T;
			else if(tag->name == Base64)
				value->type = Base64_T;
			/* Value with no type is automatically a string */
			else if(tag->name == Value)
				value->type = String_T;

			/* set value data - If data chunk already exists.. append to it.
			   It could be that data is already set for non scalar types
			   (e.g struct or array).. We'll recongnize those cases because
			   the data_len should be set to 0. */
			if(value->data && (value->data_len > 0)) {
				value->data = g_realloc(value->data, value->data_len + namelen + 1);
				memcpy(((value->data)+(value->data_len)), name, namelen);
				value->data_len = value->data_len + namelen;
				((char*)(value->data))[value->data_len] = '\0';
				xmlrpc_debug("Appending value data\n");
			}
			else if (!(value->data)) {
				value->data = g_memdup(name, namelen+1);
				value->data_len = namelen;
				((char*)(value->data))[value->data_len] = '\0';
			}
			else
				xmlrpc_debug("Got junk after non-scalar value.. ignoring\n");
			if(value->data && value->data_len>0)
				xmlrpc_debug("Char element for %d tag\n", tag->name);
			break;
		}
		case Name: {
			struct xmlrpc_struct_member *member;
			
			/* Copy name to struct member name. Assuming there won't be any newline
			   chars between name tags.*/
			CHECK_POINTER(tag->data);
			member = XMLRPC_STRUCT_MEMBER(tag->data);
			member->name = g_memdup(name, namelen+1);
			(member->name)[namelen] = '\0';
			xmlrpc_debug("Char element: %s\n", member->name);
			break;
		}
		case MethodResponse:
		case Params:
		case Param:
		case Struct:
		case Member:
		case Array:
		case Data:
		case Fault:
		case Unknown:
		}/*end switch*/
		break;
	default:
		break;
	}/* end switch */
}/*end state_machine()*/

#undef CHECK_POINTER

static void start_event(void *userdata, const char *tag, const char **attr) {
	state_machine(start_element, userdata, tag, strlen(tag), attr);
}

static void end_event(void *userdata, const char *tag) {
	state_machine(end_element, userdata, tag, strlen(tag), 0);
}

static void char_event(void *userdata, const char *txt, int txtlen) {
	state_machine(char_element, userdata, txt, txtlen, 0);
}

static void proc_event(void *data, const char *target, const char *pidata) {
	xmlrpc_debug("xml proc. inst.: %s\n", target);
}

static void xmlrpc_free_item(parse_state item, void* data) {
	if(!data)
		return;

	switch(item) {
	case Param: {
		struct xmlrpc_param *param = XMLRPC_PARAM(data);
		xmlrpc_free_item(Value, param->value);
		g_free(param);
		break;
	}
	case Value: {
		struct xmlrpc_value *value = XMLRPC_VALUE(data);
		
		switch(value->type) {
		case Struct_T:
			xmlrpc_free_item(Struct, value->data);
			break;
		case Array_T:
			xmlrpc_free_item(Array, value->data);
			break;
		case Integer_T:
		case Boolean_T:
		case String_T:
		case Double_T:
		case DateTime_iso8601_T:
		case Base64_T:
			g_free(value->data);
			break;
		default:
		}
		g_free(value);
		break;
	}
	case Struct: {
		struct xmlrpc_struct *s = XMLRPC_STRUCT(data);
		xmlrpc_free_item(Member, (void*)s->member);
		xmlrpc_free_item(Struct, (void*)s->next);
		g_free(s);
		break;
	}
	case Member: {
		struct xmlrpc_struct_member *m = XMLRPC_STRUCT_MEMBER(data);
		g_free(m->name);
		xmlrpc_free_item(Value, (void*)m->value);
		g_free(m);
		break;
	}
	case Array: {
		struct xmlrpc_array *a = XMLRPC_ARRAY(data);
		xmlrpc_free_item(Data, (void*)a->data);
		g_free(a);
		break;
	}
	case Data: {
		struct xmlrpc_data *d = XMLRPC_DATA(data);
		xmlrpc_free_item(Value, (void*)d->value);
		xmlrpc_free_item(Data, (void*)d->next);
		g_free(d);
		break;
	}
	case Fault: {
		struct xmlrpc_fault *f = XMLRPC_FAULT(data);
		xmlrpc_free_item(Value, (void*)f->value);
		g_free(f);
		break;
	}
	case MethodResponse:
	case Params:
	case Integer:
	case Boolean:
	case String:
	case Double:
	case DateTime_iso8601:
	case Base64:
	case Name:
	case Unknown:
	default:
	}/* end switch */
}/* end xmlrpc_free_item() */

/*
 *  PUBLIC CODE
 */

void xmlrpc_free_response(struct xmlrpc_response *resp) {
	if(!resp)
		return;

	if(resp->type == valid)
		xmlrpc_free_item(Param, resp->data);
	else if(resp->type == fault)
		xmlrpc_free_item(Fault, resp->data);
	else
		xmlrpc_debug("Unkown xmlrpc_response type: %d\n", resp->type);
	g_free(resp);
}

static void xmlrpc_free_tagstack(stack *s) {
	struct xmlrpc_tag *tag;

	while((tag=stack_pop(s)))
		g_free(tag);
	g_free(s);
}

struct xmlrpc_response *xmlrpc_parse(const char *xml_buffer) {
	XML_Parser p = XML_ParserCreate(NULL);
	struct state_data *sd = g_new0(struct state_data, 1);
	struct xmlrpc_response *ret=NULL;

	/* initialize state data */
	sd->tag_stack = stack_new();
	sd->error = 0;

	if (! p) {
		gaim_debug(GAIM_DEBUG_WARNING, "blogger", "xmlrpc: Error - Couldn't allocate memory for parser\n");
		return NULL;
	}
	
	XML_SetElementHandler(p, start_event, end_event);
	XML_SetCharacterDataHandler(p, char_event);
	XML_SetProcessingInstructionHandler(p, proc_event);
	XML_SetUserData(p, (void*)sd);

	if (!XML_Parse(p, xml_buffer, strlen(xml_buffer), 1) || sd->error) {
		if(!sd->error) {
			gaim_debug(GAIM_DEBUG_WARNING, "blogger", "xmlrpc: Error - xml parse error at line %d:\n%s\n",
				     XML_GetCurrentLineNumber(p),
				     XML_ErrorString(XML_GetErrorCode(p)));
		}
		xmlrpc_free_response(sd->response);
		xmlrpc_free_tagstack(sd->tag_stack);
		g_free(sd);
		/* Dump the xml for debugging */
		gaim_debug(GAIM_DEBUG_INFO, "blogger", "xmlrpc: Error parsing xmlrpc.. Dumping xml..:\n", xml_buffer);
		return NULL;
	}
	ret = sd->response;
	stack_free(sd->tag_stack);
	g_free(sd);
	gaim_debug(GAIM_DEBUG_INFO, "blogger", "Success parsing xmlrpc\n");

	return ret;
}

