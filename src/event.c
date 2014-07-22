#include "lstage.h"
#include "event.h"
#include "stage.h"
#include "instance.h"
#include "threading.h"
#include "scheduler.h"
#include "marshal.h"

#include <string.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <unistd.h>

static THREAD_T *event_thread;
static struct event_base *loop;

event_t
lstage_newevent (const char *ev, size_t len)
{
	event_t e = malloc (sizeof (struct event_s));
	e->data = malloc (len);
	memcpy (e->data, ev, len);
	e->len = len;
	return e;
}

void
lstage_destroyevent (event_t e)
{
	free (e->data);
	free (e);
}

static void
dummy_event (evutil_socket_t fd, short events, void *arg)
{
}

static void
io_ready (evutil_socket_t fd, short event, void *arg)
{
	instance_t i = (instance_t) arg;
	if (event & EV_TIMEOUT)
		i->flags = I_TIMEOUT_IO;
	lstage_pushinstance (i);
}

static int
event_wait_io (lua_State * L)
{
	int fd = -1;
	fd = lua_tointeger (L, 1);
	int mode = -1;
	mode = lua_tointeger (L, 2);
	int m = 0;
	if (mode == 0)
		m = EV_READ;	// read
	else if (mode == 1)
		m = EV_WRITE;	//write
	else
		luaL_error (L,
			    "Invalid io operation type (0=read and 1=write)");
	double time = 0.0l;
	if (lua_type (L, 3) == LUA_TNUMBER)
	  {
		  time = lua_tonumber (L, 3);
		  if (time <= 0.0l)
			  luaL_error (L, "Invalid timeout value");
	  }
	lua_pushliteral (L, LSTAGE_INSTANCE_KEY);
	lua_gettable (L, LUA_REGISTRYINDEX);
	if (lua_type (L, -1) != LUA_TLIGHTUSERDATA)
		luaL_error (L, "Cannot wait outside of a stage");
	instance_t i = lua_touserdata (L, -1);
	lua_pop (L, 1);
	i->flags = I_WAITING_IO;
	if (time > 0.0)
	  {
		  struct timeval to =
			  { time,
		      (((double) time - ((int) time)) * 1000000.0L) };
		  event_base_once (loop, fd, m, io_ready, i, &to);
	  }
	else
	  {
		  event_base_once (loop, fd, m, io_ready, i, NULL);
	  }
	return lua_yield (L, 0);
}

static int
event_sleep (lua_State * L)
{
	double time = 0.0l;
	time = lua_tonumber (L, 1);
	if (time < 0.0L)
		luaL_error (L, "Invalid time (negative)");
	lua_pushliteral (L, LSTAGE_INSTANCE_KEY);
	lua_gettable (L, LUA_REGISTRYINDEX);
	if (lua_type (L, -1) != LUA_TLIGHTUSERDATA)
	  {
		  usleep ((useconds_t) (time * 1000000.0L));
		  lua_pop (L, 1);
		  return 0;
	  }
	instance_t i = lua_touserdata (L, -1);
	lua_pop (L, 1);
	i->flags = I_WAITING_IO;
	struct timeval to =
		{ time, (((double) time - ((int) time)) * 1000000.0L) };
	event_base_once (loop, -1, EV_TIMEOUT, io_ready, i, &to);
	return lua_yield (L, 0);
}

static THREAD_RETURN_T THREAD_CALLCONV
event_main (void *t_val)
{
	loop = event_base_new ();
	if (!loop)
		return NULL;
	struct event *listener_event =
		event_new (loop, -1, EV_READ | EV_PERSIST, dummy_event, NULL);
	event_add (listener_event, NULL);
	event_base_dispatch (loop);
	return NULL;
}

int
lstage_restoreevent (lua_State * L, event_t ev)
{
//      stackDump(L,"");
	lua_pushcfunction (L, mar_decode);
	lua_pushlstring (L, ev->data, ev->len);
	lua_call (L, 1, 1);
//      stackDump(L,"");
	int top = lua_gettop (L);
	int n =
#if LUA_VERSION_NUM < 502
		luaL_getn (L, -1);
#else
		luaL_len (L, -1);
#endif
	int j;
	for (j = 1; j <= n; j++)
		lua_rawgeti (L, top, j);
	lua_remove (L, top);
	return n;
}

LSTAGE_EXPORTAPI int
luaopen_lstage_event (lua_State * L)
{
	const struct luaL_Reg LuaExportFunctions[] = {
		{"encode", mar_encode},
		{"decode", mar_decode},
		{"waitfd", event_wait_io},
		{"sleep", event_sleep},
		{NULL, NULL}
	};
	if (!event_thread)
	  {
		  event_thread = malloc (sizeof (THREAD_T));
		  evthread_use_pthreads ();
		  THREAD_CREATE (event_thread, &event_main, NULL, 0);
	  }
	lua_newtable (L);
	lua_newtable (L);
	luaL_loadstring (L,
			 "return function() return require'lstage.event' end");
	lua_setfield (L, -2, "__persist");
	lua_setmetatable (L, -2);
	lua_pushliteral (L, "READ");
	lua_pushnumber (L, 0);
	lua_settable (L, -3);
	lua_pushliteral (L, "WRITE");
	lua_pushnumber (L, 1);
	lua_settable (L, -3);
#if LUA_VERSION_NUM < 502
	luaL_register (L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs (L, LuaExportFunctions, 0);
#endif
	return 1;
};
