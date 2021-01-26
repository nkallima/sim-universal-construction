#include <uthreads.h>

inline static void switch_to_fiber(Fiber *prev, Fiber *cur);
inline static void fiber_start_func(FiberData *context);

static volatile __thread int N_FIBERS = 1;
static __thread Fiber *FIBER_LIST = null;
static __thread Fiber *FIBER_RECYCLE = null;
static __thread int MAX_FIBERS = 1;
static __thread int currentFiber = 0;


void initFibers(int max) {
    int i;

    MAX_FIBERS = max;
    FIBER_LIST = getMemory(MAX_FIBERS * sizeof(Fiber));
    getcontext(&(FIBER_LIST[0].context));
    FIBER_LIST[0].active = true;
    FIBER_LIST[0].context.uc_link = NULL;
    for (i = 1; i < MAX_FIBERS; i++)
        FIBER_LIST[i].active = false;
}

inline static void switch_to_fiber(Fiber *prev, Fiber *cur) {
    if (_setjmp(prev->jmp) == 0) {
        _longjmp(cur->jmp, 1);
    }
}

void fiberYield(void) {
    int prev_fiber;

    if (N_FIBERS == 1)
        return;
    // Saved the state so call the next fiber
    prev_fiber = currentFiber;
    do {
        currentFiber = (currentFiber + 1) % MAX_FIBERS;
    } while (FIBER_LIST[currentFiber].active == false);
    switch_to_fiber(&FIBER_LIST[prev_fiber], &FIBER_LIST[currentFiber]);
    if (FIBER_RECYCLE != null) {
        freeMemory(FIBER_RECYCLE, sizeof(Fiber));
        FIBER_RECYCLE = null;
    }
}

static void fiber_start_func(FiberData *context) {
    ucontext_t tmp;
    void *(*func)(void *);
    long arg;
    int prev_fiber;

    func = context->func;
    arg = context->arg;
    if (_setjmp(*(context->cur)) == 0)
        swapcontext(&tmp, context->prev);

    func((void *)arg); // Execute fiber with function func
    if (N_FIBERS == 1) // This check is not useful, since
        return;        // main thread never calls fiber_start_func
    FIBER_RECYCLE = FIBER_LIST[currentFiber].context.uc_stack.ss_sp;
    FIBER_LIST[currentFiber].active = false;
    N_FIBERS--;
    prev_fiber = currentFiber;
    do {
        currentFiber = (currentFiber + 1) % MAX_FIBERS;
    } while (FIBER_LIST[currentFiber].active == false);
    switch_to_fiber(&FIBER_LIST[prev_fiber], &FIBER_LIST[currentFiber]);
}

int spawnFiber(void *(*func)(void *), long arg) {
    ucontext_t tmp;
    FiberData context;

    if (N_FIBERS == MAX_FIBERS)
        return -1;

    getcontext(&(FIBER_LIST[N_FIBERS].context));
    FIBER_LIST[N_FIBERS].active = true;
    FIBER_LIST[N_FIBERS].context.uc_link = NULL;
    FIBER_LIST[N_FIBERS].context.uc_stack.ss_sp = getMemory(FIBER_STACK);
    FIBER_LIST[N_FIBERS].context.uc_stack.ss_size = FIBER_STACK;
    FIBER_LIST[N_FIBERS].context.uc_stack.ss_flags = 0;

    context.func = func;
    context.arg = arg;
    context.cur = &FIBER_LIST[N_FIBERS].jmp;
    context.prev = &tmp;
    makecontext(&FIBER_LIST[N_FIBERS].context, (void (*)())fiber_start_func, 1, &context);
    swapcontext(&tmp, &FIBER_LIST[N_FIBERS].context);
    N_FIBERS++;
    return 1;
}

void waitForAllFibers(void) { // Execute the fibers until they quit
    while (N_FIBERS > 1)
        fiberYield();
}
