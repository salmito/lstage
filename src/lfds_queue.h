#ifndef lf_queue_H_
#define lf_queue_H_

typedef struct RingQueue * RingQueue_t;
typedef struct RingQueue * LFQueue_t;

#define lstage_lfqueue_try_push(q,p) lstage_lfqueue_try_push_((q),(void **)(p))
#define lstage_lfqueue_try_pop(q,p) lstage_lfqueue_try_pop_((q),(void **)(p))

RingQueue_t lstage_lfqueue_new();
int lstage_lfqueue_try_push_(RingQueue_t q,void ** source);
void lstage_lfqueue_push(RingQueue_t q,void ** source);
int lstage_lfqueue_try_pop_(RingQueue_t q, void ** destination);
void lstage_lfqueue_pop(RingQueue_t q, void ** destination);
void lstage_lfqueue_setcapacity(RingQueue_t q, int capacity);
int lstage_lfqueue_getcapacity(RingQueue_t q);
int lstage_lfqueue_isempty(RingQueue_t q);
int lstage_lfqueue_size(RingQueue_t q);

//thread unsafe
void lstage_lfqueue_free(RingQueue_t q);


#endif //lf_queue_H_
