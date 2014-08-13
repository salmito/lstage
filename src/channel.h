#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include "lstage.h"
#include "lf_queue.h"
#include "threading.h"

typedef struct channel_s  * channel_t;

#define LSTAGE_CHANNEL_CACHE "lstage-channel-cache"

#include "event.h"

struct channel_s {
	LFqueue_t event_queue;
	LFqueue_t wait_queue;
	MUTEX_T mutex;
	SIGNAL_T cond;
	volatile int waiting;
	event_t event;
	int lock;
};

//channel_t * lstage_newchannel(lua_State *L);
channel_t lstage_tochannel(lua_State *L, int i);
int lstage_channelnew(lua_State *L);
void lstage_pushchannel(lua_State * L,channel_t t);
int lstage_pushevent(lua_State *L);

#endif
