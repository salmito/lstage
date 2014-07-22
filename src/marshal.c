/*
* Copyright (c) 2010 Richard Hundt
*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "marshal.h"

#if LUA_VERSION_NUM > 501
#define lua_objlen lua_rawlen
#define LSTAGE_ENV_MARKER "__lstage-env-5.2-mark"
#endif

typedef struct mar_Buffer
{
	size_t size;
	size_t seek;
	size_t head;
	char *data;
} mar_Buffer;

static int mar_encode_table (lua_State * L, mar_Buffer * buf, size_t * idx,
			     int nowrap);
static int mar_decode_table (lua_State * L, const char *buf, size_t len,
			     size_t * idx);

static void
buf_init (lua_State * L, mar_Buffer * buf)
{
	buf->size = 128;
	buf->seek = 0;
	buf->head = 0;
	if (!(buf->data = malloc (buf->size)))
		luaL_error (L, "Out of memory!");
}

static void
buf_done (lua_State * L, mar_Buffer * buf)
{
	free (buf->data);
}

static int
buf_write (lua_State * L, const char *str, size_t len, mar_Buffer * buf)
{
	if (len > UINT32_MAX)
		luaL_error (L, "buffer too long");
	if (buf->size - buf->head < len)
	  {
		  size_t new_size = buf->size << 1;
		  size_t cur_head = buf->head;
		  while (new_size - cur_head <= len)
		    {
			    new_size = new_size << 1;
		    }
		  if (!(buf->data = realloc (buf->data, new_size)))
		    {
			    luaL_error (L, "Out of memory!");
		    }
		  buf->size = new_size;
	  }
	memcpy (&buf->data[buf->head], str, len);
	buf->head += len;
	return 0;
}

static const char *
buf_read (lua_State * L, mar_Buffer * buf, size_t * len)
{
	if (buf->seek < buf->head)
	  {
		  buf->seek = buf->head;
		  *len = buf->seek;
		  return buf->data;
	  }
	*len = 0;
	return NULL;
}

static void
mar_encode_value (lua_State * L, mar_Buffer * buf, int val, size_t * idx,
		  int nowrap)
{
	size_t l;
	int val_type = lua_type (L, val);
	lua_pushvalue (L, val);

	buf_write (L, (void *) &val_type, MAR_CHR, buf);
	switch (val_type)
	  {
	  case LUA_TBOOLEAN:
		  {
			  int int_val = lua_toboolean (L, -1);
			  buf_write (L, (void *) &int_val, MAR_CHR, buf);
			  break;
		  }
	  case LUA_TSTRING:
		  {
			  const char *str_val = lua_tolstring (L, -1, &l);
			  buf_write (L, (void *) &l, MAR_I32, buf);
			  buf_write (L, str_val, l, buf);
			  break;
		  }
	  case LUA_TNUMBER:
		  {
			  lua_Number num_val = lua_tonumber (L, -1);
			  buf_write (L, (void *) &num_val, MAR_I64, buf);
			  break;
		  }
	  case LUA_TLIGHTUSERDATA:
		  {
			  if (nowrap)
				  luaL_error (L,
					      "light userdata not permitted");
			  void *ptr_val = lua_touserdata (L, -1);
			  long long v = (long long) ptr_val;
			  buf_write (L, (char *) &v, MAR_I64, buf);
			  break;
		  }
	  case LUA_TTABLE:
		  {
			  int tag, ref;
			  lua_pushvalue (L, -1);
			  lua_rawget (L, SEEN_IDX);
			  if (!lua_isnil (L, -1))
			    {
				    ref = lua_tointeger (L, -1);
				    tag = MAR_TREF;
				    buf_write (L, (void *) &tag, MAR_CHR,
					       buf);
				    buf_write (L, (void *) &ref, MAR_I32,
					       buf);
				    lua_pop (L, 1);
			    }
			  else
			    {
				    mar_Buffer rec_buf;
				    lua_pop (L, 1);	/* pop nil */
				    if (luaL_getmetafield
					(L, -1, "__persist"))
				      {
					      tag = MAR_TUSR;

					      lua_pushvalue (L, -2);	/* self */
					      lua_call (L, 1, 1);
					      if (!lua_isfunction (L, -1))
						{
							luaL_error (L,
								    "__persist must return a function");
						}

					      lua_remove (L, -2);	/* __persist */

					      lua_newtable (L);
					      lua_pushvalue (L, -2);	/* callback */
					      lua_rawseti (L, -2, 1);

					      buf_init (L, &rec_buf);
					      mar_encode_table (L, &rec_buf,
								idx, nowrap);

					      buf_write (L, (void *) &tag,
							 MAR_CHR, buf);
					      buf_write (L,
							 (void *) &rec_buf.
							 head, MAR_I32, buf);
					      buf_write (L, rec_buf.data,
							 rec_buf.head, buf);
					      buf_done (L, &rec_buf);
					      lua_pop (L, 1);
				      }
				    else
				      {
					      tag = MAR_TVAL;

					      lua_pushvalue (L, -1);
					      lua_pushinteger (L, (*idx)++);
					      lua_rawset (L, SEEN_IDX);

					      lua_pushvalue (L, -1);
					      buf_init (L, &rec_buf);
					      mar_encode_table (L, &rec_buf,
								idx, nowrap);
					      lua_pop (L, 1);

					      buf_write (L, (void *) &tag,
							 MAR_CHR, buf);
					      buf_write (L,
							 (void *) &rec_buf.
							 head, MAR_I32, buf);
					      buf_write (L, rec_buf.data,
							 rec_buf.head, buf);
					      buf_done (L, &rec_buf);
				      }
			    }
			  break;
		  }
	  case LUA_TFUNCTION:
		  {
			  int tag, ref;
			  lua_pushvalue (L, -1);
			  lua_rawget (L, SEEN_IDX);
			  if (!lua_isnil (L, -1))
			    {
				    ref = lua_tointeger (L, -1);
				    tag = MAR_TREF;
				    buf_write (L, (void *) &tag, MAR_CHR,
					       buf);
				    buf_write (L, (void *) &ref, MAR_I32,
					       buf);
				    lua_pop (L, 1);
			    }
			  else
			    {
				    mar_Buffer rec_buf;
				    int i;
				    lua_Debug ar;
				    lua_pop (L, 1);	/* pop nil */

				    lua_pushvalue (L, -1);
				    lua_getinfo (L, ">nuS", &ar);
				    //printf("Function name='%s' type='%s' nups=%d\n",ar.namewhat,ar.what,ar.nups);
				    if (ar.what[0] != 'L')
				      {
					      luaL_error (L,
							  "attempt to persist a C function '%s'",
							  ar.name);
				      }
				    tag = MAR_TVAL;
				    lua_pushvalue (L, -1);
				    lua_pushinteger (L, (*idx)++);
				    lua_rawset (L, SEEN_IDX);

				    lua_pushvalue (L, -1);
				    buf_init (L, &rec_buf);
				    lua_dump (L, (lua_Writer) buf_write,
					      &rec_buf);

				    buf_write (L, (void *) &tag, MAR_CHR,
					       buf);
				    buf_write (L, (void *) &rec_buf.head,
					       MAR_I32, buf);
				    buf_write (L, rec_buf.data, rec_buf.head,
					       buf);
				    buf_done (L, &rec_buf);
				    lua_pop (L, 1);

				    lua_newtable (L);
				    for (i = 1; i <= ar.nups; i++)
				      {
#if LUA_VERSION_NUM > 501
					      const char *str =
						      lua_getupvalue (L, -2,
								      i);
					      if (!strncmp (str, "_ENV", 4))
						{
							//printf("Stripping _ENV\n");
							lua_pop (L, 1);
							lua_pushliteral (L,
									 LSTAGE_ENV_MARKER);
						}
#else
					      lua_getupvalue (L, -2, i);
#endif
					      lua_rawseti (L, -2, i);
				      }
				    buf_init (L, &rec_buf);
				    mar_encode_table (L, &rec_buf, idx,
						      nowrap);

				    buf_write (L, (void *) &rec_buf.head,
					       MAR_I32, buf);
				    buf_write (L, rec_buf.data, rec_buf.head,
					       buf);
				    buf_done (L, &rec_buf);
				    lua_pop (L, 1);
			    }

			  break;
		  }
	  case LUA_TUSERDATA:
		  {
			  int tag, ref;
			  lua_pushvalue (L, -1);
			  lua_rawget (L, SEEN_IDX);
			  if (!lua_isnil (L, -1))
			    {
				    ref = lua_tointeger (L, -1);
				    tag = MAR_TREF;
				    buf_write (L, (void *) &tag, MAR_CHR,
					       buf);
				    buf_write (L, (void *) &ref, MAR_I32,
					       buf);
				    lua_pop (L, 1);
			    }
			  else
			    {
				    mar_Buffer rec_buf;
				    lua_pop (L, 1);	/* pop nil */
				    if (!nowrap
					&& luaL_getmetafield (L, -1,
							      "__wrap"))
				      {
					      tag = MAR_TUSR;

					      lua_pushvalue (L, -2);
					      lua_pushinteger (L, (*idx)++);
					      lua_rawset (L, SEEN_IDX);

					      lua_pushvalue (L, -2);
					      lua_call (L, 1, 1);
					      if (!lua_isfunction (L, -1))
						{
							luaL_error (L,
								    "__wrap must return a function");
						}
					      lua_newtable (L);
					      lua_pushvalue (L, -2);
					      lua_rawseti (L, -2, 1);
					      lua_remove (L, -2);

					      buf_init (L, &rec_buf);
					      mar_encode_table (L, &rec_buf,
								idx, nowrap);

					      buf_write (L, (void *) &tag,
							 MAR_CHR, buf);
					      buf_write (L,
							 (void *) &rec_buf.
							 head, MAR_I32, buf);
					      buf_write (L, rec_buf.data,
							 rec_buf.head, buf);
					      buf_done (L, &rec_buf);
				      }
				    else if (luaL_getmetafield
					     (L, -1, "__persist"))
				      {
					      tag = MAR_TUSR;

					      lua_pushvalue (L, -2);
					      lua_pushinteger (L, (*idx)++);
					      lua_rawset (L, SEEN_IDX);

					      lua_pushvalue (L, -2);
					      lua_call (L, 1, 1);
					      if (!lua_isfunction (L, -1))
						{
							luaL_error (L,
								    "__persist must return a function");
						}
					      lua_newtable (L);
					      lua_pushvalue (L, -2);
					      lua_rawseti (L, -2, 1);
					      lua_remove (L, -2);

					      buf_init (L, &rec_buf);
					      mar_encode_table (L, &rec_buf,
								idx, nowrap);

					      buf_write (L, (void *) &tag,
							 MAR_CHR, buf);
					      buf_write (L,
							 (void *) &rec_buf.
							 head, MAR_I32, buf);
					      buf_write (L, rec_buf.data,
							 rec_buf.head, buf);
					      buf_done (L, &rec_buf);
				      }
				    else
				      {
					      luaL_error (L,
							  "attempt to encode userdata (no __wrap hook - or not permited - and no __persist hook)");
				      }
				    lua_pop (L, 1);
			    }
			  break;
		  }
	  case LUA_TNIL:
		  break;
	  default:
		  luaL_error (L, "invalid value type (%s)",
			      lua_typename (L, val_type));
	  }
	lua_pop (L, 1);
}

static int
mar_encode_table (lua_State * L, mar_Buffer * buf, size_t * idx, int nowrap)
{
	lua_pushnil (L);
	while (lua_next (L, -2) != 0)
	  {
		  mar_encode_value (L, buf, -2, idx, nowrap);
		  mar_encode_value (L, buf, -1, idx, nowrap);
		  lua_pop (L, 1);
	  }
	return 1;
}

#define mar_incr_ptr(l) \
    if (((*p)-buf)+(l) > len) luaL_error(L, "bad code"); (*p) += (l);

#define mar_next_len(l,T) \
    if (((*p)-buf)+sizeof(T) > len) luaL_error(L, "bad code"); \
    l = *(T*)*p; (*p) += sizeof(T);

static void mar_decode_value
	(lua_State * L, const char *buf, size_t len, const char **p,
	 size_t * idx)
{
	size_t l;
	char val_type = **p;
	mar_incr_ptr (MAR_CHR);
	switch (val_type)
	  {
	  case LUA_TBOOLEAN:
		  lua_pushboolean (L, *(char *) *p);
		  mar_incr_ptr (MAR_CHR);
		  break;
	  case LUA_TNUMBER:
		  lua_pushnumber (L, *(lua_Number *) * p);
		  mar_incr_ptr (MAR_I64);
		  break;
	  case LUA_TLIGHTUSERDATA:
		  {
			  void *ptr = (void *) *(void **) *p;
			  lua_pushlightuserdata (L, ptr);
			  mar_incr_ptr (MAR_I64);
		  } break;
	  case LUA_TSTRING:
		  mar_next_len (l, uint32_t);
		  lua_pushlstring (L, *p, l);
		  mar_incr_ptr (l);
		  break;
	  case LUA_TTABLE:
		  {
			  char tag = *(char *) *p;
			  mar_incr_ptr (MAR_CHR);
			  if (tag == MAR_TREF)
			    {
				    int ref;
				    mar_next_len (ref, int);
				    lua_rawgeti (L, SEEN_IDX, ref);
			    }
			  else if (tag == MAR_TVAL)
			    {
				    mar_next_len (l, uint32_t);
				    lua_newtable (L);
				    lua_pushvalue (L, -1);
				    lua_rawseti (L, SEEN_IDX, (*idx)++);
				    mar_decode_table (L, *p, l, idx);
				    mar_incr_ptr (l);
			    }
			  else if (tag == MAR_TUSR)
			    {
				    mar_next_len (l, uint32_t);
				    lua_newtable (L);
				    mar_decode_table (L, *p, l, idx);
				    lua_rawgeti (L, -1, 1);
				    lua_call (L, 0, 1);
				    lua_remove (L, -2);
				    lua_pushvalue (L, -1);
				    lua_rawseti (L, SEEN_IDX, (*idx)++);
				    mar_incr_ptr (l);
			    }
			  else
			    {
				    luaL_error (L, "bad encoded data");
			    }
			  break;
		  }
	  case LUA_TFUNCTION:
		  {
			  size_t nups;
			  int i;
			  mar_Buffer dec_buf;
			  char tag = *(char *) *p;
			  mar_incr_ptr (1);
			  if (tag == MAR_TREF)
			    {
				    int ref;
				    mar_next_len (ref, int);
				    lua_rawgeti (L, SEEN_IDX, ref);
			    }
			  else
			    {
				    mar_next_len (l, uint32_t);
				    dec_buf.data = (char *) *p;
				    dec_buf.size = l;
				    dec_buf.head = l;
				    dec_buf.seek = 0;
				    lua_load (L, (lua_Reader) buf_read,
					      &dec_buf, "=marshal"
#if LUA_VERSION_NUM > 501
					      , NULL
#endif
					    );
				    mar_incr_ptr (l);

				    lua_pushvalue (L, -1);
				    lua_rawseti (L, SEEN_IDX, (*idx)++);

				    mar_next_len (l, uint32_t);
				    lua_newtable (L);
				    mar_decode_table (L, *p, l, idx);
				    nups = lua_objlen (L, -1);
				    for (i = 1; i <= nups; i++)
				      {
					      lua_rawgeti (L, -1, i);
#if LUA_VERSION_NUM > 501
					      if (lua_type (L, -1) ==
						  LUA_TSTRING)
						{
							size_t len = 0;
							const char *s =
								lua_tolstring
								(L, -1, &len);
							if (!strncmp
							    (s,
							     LSTAGE_ENV_MARKER,
							     sizeof
							     (LSTAGE_ENV_MARKER)))
							  {
								  lua_rawgeti
									  (L,
									   LUA_REGISTRYINDEX,
									   LUA_RIDX_GLOBALS);
								  lua_remove
									  (L,
									   -2);
							  }
						}
#endif
					      lua_setupvalue (L, -3, i);
				      }
				    lua_pop (L, 1);
				    mar_incr_ptr (l);
			    }
			  break;
		  }
	  case LUA_TUSERDATA:
		  {
			  char tag = *(char *) *p;
			  mar_incr_ptr (MAR_CHR);
			  if (tag == MAR_TREF)
			    {
				    int ref;
				    mar_next_len (ref, int);
				    lua_rawgeti (L, SEEN_IDX, ref);
			    }
			  else if (tag == MAR_TUSR)
			    {
				    mar_next_len (l, uint32_t);
				    lua_newtable (L);
				    mar_decode_table (L, *p, l, idx);
				    lua_rawgeti (L, -1, 1);
				    lua_call (L, 0, 1);
				    lua_remove (L, -2);
				    lua_pushvalue (L, -1);
				    lua_rawseti (L, SEEN_IDX, (*idx)++);
				    mar_incr_ptr (l);
			    }
			  else
			    {	/* tag == MAR_TVAL */
				    lua_pushnil (L);
			    }
			  break;
		  }
	  case LUA_TNIL:
	  case LUA_TTHREAD:
		  lua_pushnil (L);
		  break;
	  default:
		  luaL_error (L, "bad code");
	  }
}

static int
mar_decode_table (lua_State * L, const char *buf, size_t len, size_t * idx)
{
	const char *p;
	p = buf;
	while (p - buf < len)
	  {
		  mar_decode_value (L, buf, len, &p, idx);
		  mar_decode_value (L, buf, len, &p, idx);
		  lua_settable (L, -3);
	  }
	return 1;
}

int
mar_encode (lua_State * L)
{
	const unsigned char m = MAR_MAGIC;
	size_t idx, len;
	mar_Buffer buf;

	if (lua_isnone (L, 1))
	  {
		  lua_pushnil (L);
	  }
	if (lua_isnone (L, 2))
	  {
		  lua_pushnil (L);
	  }
	if (lua_isnil (L, 2))
	  {
		  lua_newtable (L);
		  lua_replace (L, 2);
	  }
	else if (!lua_istable (L, 2))
	  {
		  luaL_error (L,
			      "bad argument #2 to encode (expected table)");
	  }
	int nowrap = 0;
	if (lua_isboolean (L, 3))
	  {
		  nowrap = lua_toboolean (L, 3);
		  lua_remove (L, 3);
	  }
	len = lua_objlen (L, 2);
	lua_newtable (L);
	for (idx = 1; idx <= len; idx++)
	  {
		  lua_rawgeti (L, 2, idx);
		  lua_pushinteger (L, idx);
		  lua_rawset (L, SEEN_IDX);
	  }

	lua_pushvalue (L, 1);

	buf_init (L, &buf);
	buf_write (L, (void *) &m, 1, &buf);
	mar_encode_value (L, &buf, -1, &idx, nowrap);
	if (nowrap)
	  {
		  lua_pushboolean (L, nowrap);
		  lua_replace (L, 3);
	  }
	lua_pop (L, 1);

	lua_pushlstring (L, buf.data, buf.head);

	buf_done (L, &buf);

	lua_remove (L, SEEN_IDX);

	return 1;
}

int
mar_decode (lua_State * L)
{
	size_t l, idx, len;
	const char *p;
	const char *s = luaL_checklstring (L, 1, &l);

	if (l < 1)
		luaL_error (L, "bad header");
	if (*(unsigned char *) s++ != MAR_MAGIC)
		luaL_error (L, "bad magic");
	l -= 1;

	if (lua_isnoneornil (L, 2))
	  {
		  lua_newtable (L);
	  }
	else if (!lua_istable (L, 2))
	  {
		  luaL_error (L,
			      "bad argument #2 to decode (expected table)");
	  }

	len = lua_objlen (L, 2);
	lua_newtable (L);
	for (idx = 1; idx <= len; idx++)
	  {
		  lua_rawgeti (L, 2, idx);
		  lua_rawseti (L, SEEN_IDX, idx);
	  }

	p = s;
	mar_decode_value (L, s, l, &p, &idx);

	lua_remove (L, SEEN_IDX);
	lua_remove (L, 2);

	return 1;
}

int
mar_clone (lua_State * L)
{
	mar_encode (L);
	lua_replace (L, 1);
	mar_decode (L);
	return 1;
}

/*static const luaL_reg R[] =
{
    {"encode",      mar_encode},
    {"decode",      mar_decode},
    {"clone",       mar_clone},
    {NULL,	    NULL}
};

int luaopen_lstage_marshal(lua_State *L)
{
    lua_newtable(L);
    luaL_register(L, NULL, R);
    return 1;
}*/
