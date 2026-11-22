#include <list.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <assert.h>

void
wakeup_proc(struct proc_struct *proc) {
    assert(proc->state != PROC_ZOMBIE && proc->state != PROC_RUNNABLE);
    proc->state = PROC_RUNNABLE;
}

//LAB4 function3 YOUR CODE

void
schedule(void)
{
    bool intr_flag;
    local_intr_save(intr_flag);

    current->need_resched = 0;

    list_entry_t *head = &proc_list;
    list_entry_t *start = (current == idleproc)
                              ? head
                              : &(current->list_link);
    struct proc_struct *next = NULL;

    for (list_entry_t *le = list_next(start);
         le != start; le = list_next(le))
    {
        if (le == head)
        {
            continue;
        }

        struct proc_struct *proc = le2proc(le, list_link);
        if (proc->state == PROC_RUNNABLE)
        {
            next = proc;
            break;
        }
    }


    if (next == NULL &&
        current != idleproc &&
        current->state == PROC_RUNNABLE)
    {
        next = current;
    }

    if (next == NULL)
    {
        next = idleproc;
    }

    next->runs++;

    if (next != current)
    {
        proc_run(next);
    }

    local_intr_restore(intr_flag);
}


