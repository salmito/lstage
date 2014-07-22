#include "lfds_queue.h"

#include <stdlib.h>

#include "disruptor.h"

DEFINE_ENTRY_TYPE (uint_fast64_t, entry_t);
DEFINE_RING_BUFFER_TYPE (MAX_ENTRY_PROCESSORS, ENTRY_BUFFER_SIZE, entry_t,
			 ring_buffer_t);
DEFINE_RING_BUFFER_MALLOC (ring_buffer_t);
DEFINE_RING_BUFFER_INIT (ENTRY_BUFFER_SIZE, ring_buffer_t);
DEFINE_RING_BUFFER_SHOW_ENTRY_FUNCTION (entry_t, ring_buffer_t);
DEFINE_RING_BUFFER_ACQUIRE_ENTRY_FUNCTION (entry_t, ring_buffer_t);
DEFINE_ENTRY_PROCESSOR_BARRIER_REGISTER_FUNCTION (ring_buffer_t);
DEFINE_ENTRY_PROCESSOR_BARRIER_UNREGISTER_FUNCTION (ring_buffer_t);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_BLOCKING_FUNCTION (ring_buffer_t);
DEFINE_ENTRY_PROCESSOR_BARRIER_RELEASEENTRY_FUNCTION (ring_buffer_t);
DEFINE_ENTRY_PUBLISHER_NEXTENTRY_BLOCKING_FUNCTION (ring_buffer_t);
DEFINE_ENTRY_PUBLISHER_NEXTENTRY_NONBLOCKING_FUNCTION (ring_buffer_t);
DEFINE_ENTRY_PUBLISHER_COMMITENTRY_BLOCKING_FUNCTION (ring_buffer_t);

typedef void *__ptr;


/* Queue internal structure (Lock Free Queue) */
struct RingQueue
{
	struct ring_buffer_t ring_buffer;
};


/* Create a new queue */
RingQueue_t
lstage_lfqueue_new ()
{
	RingQueue_t q;
	q = malloc (sizeof (struct RingQueue));
	ring_buffer_init (&q->ring_buffer);
	return q;
}

void
lstage_lfqueue_push (RingQueue_t q, void **source)
{	struct cursor_t cursor;
	struct entry_t *entry;


      again:
	if (!publisher_next_entry_nonblocking (&q->ring_buffer, &cursor))
		goto again2;
	entry = ring_buffer_acquire_entry (&q->ring_buffer, &cursor);
	entry->content = *source;
	publisher_commit_entry_blocking (&q->ring_buffer, &cursor);
}

int
lstage_lfqueue_try_push_ (RingQueue_t q, void **source)
{
	struct cursor_t cursor;
	struct entry_t *entry;


      again:
	if (!publisher_next_entry_nonblocking (&q->ring_buffer, &cursor))
		goto again2;
	entry = ring_buffer_acquire_entry (&q->ring_buffer, &cursor);
	entry->content = *source;
	publisher_commit_entry_blocking (&q->ring_buffer, &cursor);
}

int
lstage_lfqueue_try_pop_ (RingQueue_t q, void **destination)
{
		struct cursor_t cursor;
	struct count_t reg_number;
	cursor.sequence = entry_processor_barrier_register(&q->ring_buffer, &reg_number);
	entry_processor_barrier_wait_for_blocking(&q->ring_buffer,	&cursor.sequence);
	entry = ring_buffer_show_entry(q->ring_buffer, &cursor);
	entry_processor_barrier_unregister(q->ring_buffer, &reg_number);
	*destination=entry->content;
	return 1;
}

void
lstage_lfqueue_pop (RingQueue_t q, void **destination)
{
	struct cursor_t cursor;
	struct count_t reg_number;
	cursor.sequence = entry_processor_barrier_register(&q->ring_buffer, &reg_number);
	entry_processor_barrier_wait_for_blocking(&q->ring_buffer,	&cursor.sequence);
	entry = ring_buffer_show_entry(q->ring_buffer, &cursor);
	entry_processor_barrier_unregister(q->ring_buffer, &reg_number);
	*destination=entry->content;
	return 1;
}

void
lstage_lfqueue_setcapacity (RingQueue_t q, int capacity)
{
	//return q->queue->set_capacity(capacity);
}

int
lstage_lfqueue_getcapacity (RingQueue_t q)
{
	return 0;		//q->queue->capacity();
}

int
lstage_lfqueue_isempty (RingQueue_t q)
{
	return 0;		//(int)q->queue->empty();
}

int
lstage_lfqueue_size (RingQueue_t q)
{
	return 0;		//q->queue->size();
}

//possibly thread unsafe
void
lstage_lfqueue_free (RingQueue_t q)
{
	if (q)
	  {
		  free (q);
		  *q = 0;
	  }
}
