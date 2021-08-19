#include <oyama.h>
#include <threadtools.h>

inline static void OyamaWait(void);

const int LOCKED = 1;
const int UNLOCKED = 0;
const int OYAMA_HELP_FACTOR = 10;

static void OyamaWait(void) {
    synchResched();
}

RetVal OyamaApplyOp(volatile OyamaStruct *l, OyamaThreadState *th_state, RetVal (*sfunc)(ArgVal, int), ArgVal arg, int pid) {
    volatile OyamaAnnounceNode *mynode = &th_state->my_node;
    volatile OyamaAnnounceNode *p;
    register OyamaAnnounceNode *tmp_next;
    int help_bound = OYAMA_HELP_FACTOR * l->nthreads;

    // Initializing node
    mynode->arg_ret = arg;
    mynode->pid = pid;
    mynode->completed = false;
    mynode->next = l->tail;
    while (!CASPTR(&l->tail, mynode->next, mynode)) { // try to insert node to the announce list
        OyamaWait();
        mynode->next = l->tail;
    }

    do {
        if (l->lock == UNLOCKED && CAS32(&l->lock, UNLOCKED, LOCKED)) {
            int counter = 0;
#ifdef DEBUG
            l->rounds++;
#endif
            while (counter < help_bound && (p = (OyamaAnnounceNode *)SWAP(&l->tail, NULL)) != NULL) {
                // Start helping all the active processes
                while (p != NULL) {
                    counter++;
#ifdef DEBUG
                    l->counter++;
#endif
                    tmp_next = (OyamaAnnounceNode *)p->next;
                    p->arg_ret = sfunc(p->arg_ret, p->pid);
                    p->completed = true;
                    p = (OyamaAnnounceNode *)tmp_next;
                }
            }
            NonTSOFence();
            // Release the lock
            l->lock = UNLOCKED;
            StoreFence();
            return mynode->arg_ret;
        } else {
            while (*((volatile bool *)&mynode->completed) == false && *((volatile int32_t *)&l->lock) == LOCKED) {
                ;
            }
            if (mynode->completed)
                return mynode->arg_ret;
        }
    } while (true);
}

void OyamaThreadStateInit(OyamaThreadState *th_state) {
    ;
}

void OyamaInit(OyamaStruct *l, uint32_t nthreads) {
    l->nthreads = nthreads;
    l->lock = UNLOCKED;
    l->tail = NULL;
#ifdef DEBUG
    l->rounds = l->counter = 0;
#endif
    FullFence();
}
