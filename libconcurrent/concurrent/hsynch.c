#include <stdio.h>

#include <hsynch.h>
#include <threadtools.h>

#ifdef SYNCH_NUMA_SUPPORT
#    include <numa.h>
#endif

#define HSYNCH_HELP_FACTOR            10
#define HSYNCH_DEFAULT_NUMA_NODE_SIZE 8

static __thread int node_of_thread = 0;

RetVal HSynchApplyOp(HSynchStruct *l, HSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    volatile HSynchNode *p;
    volatile HSynchNode *cur;
    register HSynchNode *next_node, *tmp_next;
    register int counter = 0;
    int help_bound = HSYNCH_HELP_FACTOR * l->nthreads;

    next_node = st_thread->next_node;
    next_node->next = NULL;
    next_node->locked = true;
    next_node->completed = false;

    cur = (volatile HSynchNode *)synchSWAP(&l->Tail[node_of_thread].ptr, next_node);
    cur->arg_ret = arg;
    cur->pid = pid;
    cur->next = (HSynchNode *)next_node;

    st_thread->next_node = (HSynchNode *)cur;

    while (cur->locked) // spinning
        synchResched();

    p = cur;            // I am not been helped
    if (cur->completed) // I have been helped
        return cur->arg_ret;
    CLHLock(l->central_lock, pid);
#ifdef DEBUG
    l->rounds++;
#endif
    while (counter < help_bound && p->next != NULL) {
        synchReadPrefetch(p->next);
        counter++;
#ifdef DEBUG
        l->counter++;
#endif
        tmp_next = p->next;
        p->arg_ret = sfunc(state, p->arg_ret, p->pid);
        synchNonTSOFence();
        p->completed = true;
        synchNonTSOFence();
        p->locked = false;
        p = tmp_next;
        if (tmp_next->next == NULL && synchGetMachineModel() != INTEL_X86_MACHINE)
            synchFullFence();
    }
    p->locked = false; // Unlock the next one
    CLHUnlock(l->central_lock, pid);

    return cur->arg_ret;
}

void HSynchThreadStateInit(HSynchStruct *l, HSynchThreadState *st_thread, int pid) {
    HSynchNode *last_node = NULL;
    uint32_t node_index = 0;

#ifdef SYNCH_NUMA_SUPPORT
    if (l->numa_policy) {
        if (synchGetPreferredCore() != -1) {
            node_of_thread = synchGetPreferredNumaNode();
            if (node_of_thread == -1)
                node_of_thread = pid / l->numa_node_size;
        }
    } else {
        int ncpus = numa_num_configured_cpus();
        if (numa_node_of_cpu(0) == numa_node_of_cpu(ncpus / 2) && ncpus > 1) {
            int actual_numa_node = synchGetPreferredNumaNode();
            int actual_per_manual = numa_num_task_nodes() / l->numa_nodes;
            if (actual_per_manual != 0)
                node_of_thread = actual_numa_node / actual_per_manual;
            else {
                int threads_per_node = l->nthreads / l->numa_nodes;
                node_of_thread = synchGetPreferredCore() / threads_per_node;
            }
        } else {
            node_of_thread = pid / l->numa_node_size;
        }
    }
#else
    node_of_thread = pid / l->numa_node_size;
#endif

    if (l->nodes[node_of_thread] == NULL) {
        HSynchNode *ptr = synchGetAlignedMemory(CACHE_LINE_SIZE, (l->numa_node_size + 2) * sizeof(HSynchNode));

        last_node = &ptr[l->numa_node_size + 1];
        last_node->next = NULL;
        last_node->locked = false;
        last_node->completed = false;

        if (synchCASPTR(&l->nodes[node_of_thread], NULL, ptr) == false) 
            synchFreeMemory(ptr, (l->numa_node_size + 2) * sizeof(HSynchNode));
    }
    last_node = l->nodes[node_of_thread] + l->numa_node_size + 1;
    synchCASPTR(&l->Tail[node_of_thread].ptr, NULL, last_node);
    node_index = synchFAA32(&l->node_indexes[node_of_thread], 1);
    st_thread->next_node = l->nodes[node_of_thread] + node_index;
#ifdef DEBUG
    fprintf(stderr, "DEBUG: thread_id: %d -- running_core: %d -- running_node: %d -- hsynch_node: %d\n",
            pid, synchGetPreferredCore(), synchGetPreferredNumaNode(), node_of_thread);
#endif
}

void HSynchStructInit(HSynchStruct *l, uint32_t nthreads, uint32_t numa_regions) {
    int i;

    if (numa_regions > nthreads)
        numa_regions = nthreads;
    l->nthreads = nthreads;
    if (numa_regions == HSYNCH_DEFAULT_NUMA_POLICY) {
        // Whenever numa_regions is equal to HSYNCH_DEFAULT_NUMA_POLICY, the user uses
        // the default number of NUMA nodes, which is equal to the number of NUMA nodes
        // that the machine provides. The information about machine's NUMA characteristics
        // is provided by the functionality of numa.h lib.
        // In case that numa_regions is different than HSYNCH_DEFAULT_NUMA_POLICY, the
        // user overrides system's default number of NUMA nodes. For example, if 
        // numa_regions = 2 and the machine is equipped with 4 NUMA nodes,then the
        // H-Synch will ignore this and will create a fictitious topology of 2 NUMA nodes. 
        // This is very useful in cases of machines that provide many NUMA nodes,
        // but each each of them is equipped with a small amount of cores. In such a 
        // case, the combining degree of H-Synch may be restricted. Thus, creating
        // a fictitious topology with restricted number of NUMA nodes gives much
        // better performance. The user usually overrides HSYNCH_DEFAULT_NUMA_POLICY
        // by setting the '-n' argument in the executable of the benchmarks.
        l->numa_policy = true;

#ifdef SYNCH_NUMA_SUPPORT
        uint32_t ncpus = numa_num_configured_cpus();

        l->numa_nodes = numa_num_task_nodes();
        l->numa_node_size = nthreads / l->numa_nodes + (nthreads % l->numa_nodes);

        if (l->numa_node_size < ncpus / l->numa_nodes)
            l->numa_node_size = ncpus / l->numa_nodes;
#else
        l->numa_node_size = HSYNCH_DEFAULT_NUMA_NODE_SIZE;
        l->numa_nodes = nthreads / l->numa_node_size + (nthreads % l->numa_node_size == 0 ? 0 : 1);
        if (l->numa_nodes == 0)
            l->numa_nodes = 1;
#endif

    } else {
        l->numa_policy = false;
        l->numa_nodes = numa_regions;
        l->numa_node_size = nthreads / l->numa_nodes;
        if (nthreads % l->numa_nodes != 0) 
            l->numa_node_size *= 2;
    }

    l->central_lock = CLHLockInit(nthreads);
    l->nodes = synchGetAlignedMemory(CACHE_LINE_SIZE, l->numa_nodes * sizeof(HSynchNode *));
    l->Tail = synchGetAlignedMemory(CACHE_LINE_SIZE, l->numa_nodes * sizeof(HSynchNodePtr));
    l->node_indexes = synchGetAlignedMemory(CACHE_LINE_SIZE, l->numa_nodes * sizeof(uint32_t));
    for (i = 0; i < l->numa_nodes; i++) {
        l->node_indexes[i] = 0;
        l->nodes[i] = NULL;
        l->Tail[i].ptr = NULL;
    }
#ifdef DEBUG
    l->rounds = l->counter = 0;
#endif
    synchStoreFence();
}
