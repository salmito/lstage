#include <stdlib.h>
#include "pool.h"
#include "scheduler.h"

#define DEFAULT_QUEUE_CAPACITY -1

static void get_metatable(lua_State * L);

pool_t lstage_topool(lua_State *L, int i) {
	pool_t * p = luaL_checkudata (L, i, LSTAGE_POOL_METATABLE);
	luaL_argcheck (L, p != NULL, i, "Pool expected");
	return *p;
}

void lstage_buildpool(lua_State * L,pool_t t) {
	pool_t *s=lua_newuserdata(L,sizeof(pool_t *));
	*s=t;
	get_metatable(L);
   lua_setmetatable(L,-2);
}

static int pool_tostring (lua_State *L) {
  pool_t * s = luaL_checkudata (L, 1, LSTAGE_POOL_METATABLE);
  lua_pushfstring (L, "Pool (%p)", *s);
  return 1;
}

static int pool_ptr(lua_State * L) {
	pool_t * s = luaL_checkudata (L, 1, LSTAGE_POOL_METATABLE);
	lua_pushlightuserdata(L,*s);
	return 1;
}

static int pool_addthread(lua_State * L) {
	pool_t s=lstage_topool(L, 1);
	thread_t * th=lstage_newthread(L,s);
	qt_hash_put(s->H,th,th);
	return 1;
}

static int pool_size(lua_State * L) {
	pool_t s = lstage_topool(L, 1);
	lua_pushinteger(L,qt_hash_count(s->H));
	return 1;
}

static void get_metatable(lua_State * L) {
	luaL_getmetatable(L,LSTAGE_POOL_METATABLE);
   if(lua_isnil(L,-1)) {
   	lua_pop(L,1);
  		luaL_newmetatable(L,LSTAGE_POOL_METATABLE);
  		lua_pushvalue(L,-1);
  		lua_setfield(L,-2,"__index");
		lua_pushcfunction (L, pool_tostring);
		lua_setfield (L, -2,"__tostring");
		luaL_loadstring(L,"local ptr=(...):ptr() return function() return require'lstage.pool'.get(ptr) end");
		lua_setfield (L, -2,"__wrap");
		lua_pushcfunction(L,pool_ptr);
  		lua_setfield(L,-2,"ptr");
		lua_pushcfunction(L,pool_size);
  		lua_setfield(L,-2,"size");
  		lua_pushcfunction(L,pool_addthread);
  		lua_setfield(L,-2,"add");
  	}
}

static int pool_new(lua_State *L) {
	int size=lua_tointeger(L,1);
	if(size<0) luaL_error(L,"Initial pool size must be greater than zero");
	pool_t p=malloc(sizeof(struct pool_s));
	p->H=qt_hash_create();
	p->ready=lstage_lfqueue_new();
	lstage_lfqueue_setcapacity(p->ready,-1);
	lstage_buildpool(L,p);
	return 1;
}

static int pool_get(lua_State * L) {
	pool_t p=lua_touserdata(L,1);
	if(p) {
		lstage_buildpool(L,p);
		return 1;
	}
	lua_pushnil(L);
	lua_pushliteral(L,"Pool is null");
	return 2;
}

//TODO Destroy pool


static const struct luaL_Reg LuaExportFunctions[] = {
		{"new",pool_new},
		{"get",pool_get},
		{NULL,NULL}
};

LSTAGE_EXPORTAPI int luaopen_lstage_pool(lua_State *L) {
	lua_newtable(L);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'lstage.pool' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif        
	return 1;
};
