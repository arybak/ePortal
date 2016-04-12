/*
 * gaim - Blogger Protocol Plugin - Blogger xmlrpc http routines
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
#ifndef _XMLRPC_HTTP_H_
#define _XMLRPC_HTTP_H_

int xmlrpc_http_post(gint fd,
		     const char* useragent,
		     const char* cont_type,
		     const char* uri, 
		     const char* content, 
		     unsigned int length);
char* xmlrpc_get_http_post_response(char* buf, int *len);

#endif /*_XMLRPC_HTTP_H_*/
