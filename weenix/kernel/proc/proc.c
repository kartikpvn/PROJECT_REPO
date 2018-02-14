/******************************************************************************/
/* Important Spring 2017 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5f2e8d450c0c5851acd538befe33744efca0f1c4f9fb5f       */
/*         3c8feabc561a99e53d4d21951738da923cd1c7bbd11b30a1afb11172f80b       */
/*         984b1acfbbf8fae6ea57e0583d2610a618379293cb1de8e1e9d07e6287e8       */
/*         de7e82f3d48866aa2009b599e92c852f7dbf7a6e573f1c7228ca34b9f368       */
/*         faaef0c0fcf294cb                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "kernel.h"
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"

#include "vm/vmmap.h"

#include "fs/vfs.h"
#include "fs/vfs_syscall.h"
#include "fs/vnode.h"
#include "fs/file.h"

proc_t *curproc = NULL; /* global */
static slab_allocator_t *proc_allocator = NULL;

static list_t _proc_list;
static proc_t *proc_initproc = NULL; /* Pointer to the init process (PID 1) */

void
proc_init()
{
        list_init(&_proc_list);
        proc_allocator = slab_allocator_create("proc", sizeof(proc_t));
        KASSERT(proc_allocator != NULL);
}

proc_t *
proc_lookup(int pid)
{
        proc_t *p;
        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                if (p->p_pid == pid) {
                        return p;
                }
        } list_iterate_end();
        return NULL;
}

list_t *
proc_list()
{
        return &_proc_list;
}

static pid_t next_pid = 0;

/**
 * Returns the next available PID.
 *
 * Note: Where n is the number of running processes, this algorithm is
 * worst case O(n^2). As long as PIDs never wrap around it is O(n).
 *
 * @return the next available PID
 */
static int
_proc_getid()
{
        proc_t *p;
        pid_t pid = next_pid;
        while (1) {
failed:
                list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                        if (p->p_pid == pid) {
                                if ((pid = (pid + 1) % PROC_MAX_COUNT) == next_pid) {
                                        return -1;
                                } else {
                                        goto failed;
                                }
                        }
                } list_iterate_end();
                next_pid = (pid + 1) % PROC_MAX_COUNT;
                return pid;
        }
}

/*
 * The new process, although it isn't really running since it has no
 * threads, should be in the PROC_RUNNING state.
 *
 * Don't forget to set proc_initproc when you create the init
 * process. You will need to be able to reference the init process
 * when reparenting processes to the init process.
 */
proc_t *
proc_create(char *name)
{

    proc_t *process = (proc_t *)slab_obj_alloc(proc_allocator);
    memset(process,0,sizeof(proc_t));   

	pid_t pid = _proc_getid(); 
    KASSERT(PID_IDLE != pid || list_empty(&_proc_list));
    dbg(DBG_PRINT, "(GRADING1A 2.a)\n");
   
    /*if(PID_IDLE != pid)*/
        KASSERT(PID_INIT != pid || PID_IDLE == curproc->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 2.a)\n");
    
    process->p_pid=pid;
	strncpy(process->p_comm,name,PROC_NAME_LEN);
        
	if(process->p_pid==PID_INIT){
        dbg(DBG_PRINT, "(GRADING1A)\n");
		proc_initproc = process;
    }

	list_init(&process->p_threads);
	list_init(&process->p_children);
	process->p_pproc = curproc;
	process->p_state = PROC_RUNNING;
    process->p_status = 0;
    process->p_pagedir = pt_create_pagedir();
	
	list_link_init(&process->p_list_link);
	list_insert_tail(&_proc_list,&process->p_list_link);
	list_link_init(&process->p_child_link);
	sched_queue_init(&process->p_wait);

	if(process->p_pid!=PID_IDLE){
        dbg(DBG_PRINT, "(GRADING1A)\n");
		list_insert_tail(&curproc->p_children,&process->p_child_link);
    } 

    if(process->p_pid>2){
        dbg(DBG_PRINT, "(GRADING2A)\n");
        process->p_cwd=curproc->p_cwd; /*Should this exclude for idle and init*/
        vref(process->p_cwd);
    }
    int fd;
    for (fd=0;fd<NFILES;fd++){
        dbg(DBG_PRINT, "(GRADING2A)\n");
        process->p_files[fd]= NULL;
    }

    process->p_brk = NULL;          
    process->p_start_brk = NULL;     
    process->p_vmmap = vmmap_create();
    process->p_vmmap->vmm_proc = process;
    return process;
}

/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS)
 *    - Cleaning up VM mappings (VM)
 *    - Waking up its parent if it is waiting
 *    - Reparenting any children to the init process
 *    - Setting its status and state appropriately
 *
 * The parent will finish destroying the process within do_waitpid (make
 * sure you understand why it cannot be done here). Until the parent
 * finishes destroying it, the process is informally called a 'zombie'
 * process.
 *
 * This is also where any children of the current process should be
 * reparented to the init process (unless, of course, the current
 * process is the init process. However, the init process should not
 * have any children at the time it exits).
 *
 * Note: You do _NOT_ have to special case the idle process. It should
 * never exit this way.
 *
 * @param status the status to exit the process with
 */
void
proc_cleanup(int status)
{
    
    KASSERT(NULL != proc_initproc);
    dbg(DBG_PRINT,"(GRADING1A 2.b)\n");
    KASSERT(1 <= curproc->p_pid);
    dbg(DBG_PRINT,"(GRADING1A 2.b)\n");
    KASSERT(NULL != curproc->p_pproc);
    dbg(DBG_PRINT,"(GRADING1A 2.b)\n");
    proc_t *parent;proc_t *child;
    parent=curproc->p_pproc;

    if(parent->p_wait.tq_size != 0){
        dbg(DBG_PRINT,"(GRADING1A)\n");
       (void)sched_wakeup_on(&parent->p_wait);
    }

    if(!list_empty(&curproc->p_children) && curproc != proc_initproc){
        dbg(DBG_PRINT, "(GRADING1C)\n");
        list_iterate_begin(&curproc->p_children,child,proc_t,p_child_link){
            child->p_pproc = proc_initproc;
            list_insert_tail(&proc_initproc->p_children,&child->p_child_link);
        }list_iterate_end();
    }

    curproc->p_status = (status);
    curproc->p_state = PROC_DEAD;

    KASSERT(NULL != curproc->p_pproc);
    dbg(DBG_PRINT,"(GRADING1A 2.b)\n");   
    
}

/*
 * This has nothing to do with signals and kill(1).
 *
 * Calling this on the current process is equivalent to calling
 * do_exit().
 *
 * In Weenix, this is only called from proc_kill_all.
 */
void
proc_kill(proc_t *p, int status)
{
    kthread_t *kthr=NULL; 
	if(p==curproc) {
        dbg(DBG_PRINT,"(GRADING1C)\n");
        do_exit(status);            
    } 
    else {
            dbg(DBG_PRINT,"(GRADING1C)\n");
         list_iterate_begin(&p->p_threads,kthr,kthread_t, kt_plink){
            kthread_cancel(kthr,&status); 
     }list_iterate_end();
        p->p_status=status;
     
    }
}

/*
 * Remember, proc_kill on the current process will _NOT_ return.
 * Don't kill direct children of the idle process.
 *
 * In Weenix, this is only called by sys_halt.
 */
void
proc_kill_all()
{
    proc_t *proc; int status=1;
    dbg(DBG_PRINT,"(GRADING1C)\n");

        list_iterate_begin(&_proc_list, proc, proc_t,p_list_link){
            if((proc->p_pid!= PID_IDLE) && (proc->p_pproc->p_pid!= PID_IDLE)){
                proc_kill(proc,status);
                list_remove(&proc->p_list_link);
            }
    } list_iterate_end();    
   
}

/*
 * This function is only called from kthread_exit.
 *
 * Unless you are implementing MTP, this just means that the process
 * needs to be cleaned up and a new thread needs to be scheduled to
 * run. If you are implementing MTP, a single thread exiting does not
 * necessarily mean that the process should be exited.
 */
void
proc_thread_exited(void *retval)
{
    dbg(DBG_PRINT, "(GRADING1A)\n");
    int ret = (int)retval;
    proc_cleanup(ret);
    dbg(DBG_PRINT, "(GRADING2A)\n");
    if(curproc->p_pid !=2)
            vput(curproc->p_cwd);
    sched_switch();


}

/* If pid is -1 dispose of one of the exited children of the current
 * process and return its exit status in the status argument, or if
 * all children of this process are still running, then this function
 * blocks on its own p_wait queue until one exits.
 *
 * If pid is greater than 0 and the given pid is a child of the
 * current process then wait for the given pid to exit and dispose
 * of it.
 *
 * If the current process has no children, or the given pid is not
 * a child of the current process return -ECHILD.
 *
 * Pids other than -1 and positive numbers are not supported.
 * Options other than 0 are not supported.
 */

pid_t
do_waitpid(pid_t pid, int options, int *status)
{
    proc_t *childproc;

    if(list_empty(&curproc->p_children) )    return -ECHILD;
    while(1){
        list_iterate_begin(&curproc->p_children,childproc,proc_t,p_child_link){
   /* dbg(DBG_PRINT,"Entering within list iterate  with childproc pid %d %s : sent pid: %d\n",childproc->p_pid,curproc->p_comm,pid);*/
            if((pid==-1) || ((pid >0) && (childproc->p_pid == pid))){
                dbg(DBG_PRINT,"(GRADING1A)\n");
step:
                if(childproc->p_state == PROC_DEAD) {
                    KASSERT(NULL != childproc);
                    dbg(DBG_PRINT,"(GRADING1A 2.c)\n");
                    KASSERT(-1 == pid || childproc->p_pid == pid);
                    dbg(DBG_PRINT,"(GRADING1A 2.c)\n");

                    kthread_t* e_thread;
                    list_iterate_begin(&childproc->p_threads,e_thread,kthread_t,kt_plink){
                    if(e_thread->kt_state==KT_EXITED){
                        KASSERT(KT_EXITED == e_thread->kt_state);
                        dbg(DBG_PRINT,"(GRADING1A 2.c)\n");
                        kthread_destroy(e_thread);
                        } 
                    } list_iterate_end();

                        list_remove(&childproc->p_child_link);
                        if(status!=NULL)
                            *status = childproc->p_status;

                        KASSERT(NULL != childproc->p_pagedir);
                        dbg(DBG_PRINT,"(GRADING1A 2.c)\n");

                        list_remove(&childproc->p_list_link);
                         slab_obj_free(proc_allocator,childproc);

                        return childproc->p_pid;
                    
                }
                if(pid>0) {
                    dbg(DBG_PRINT,"(GRADING1C)\n");
                    sched_cancellable_sleep_on(&curproc->p_wait);
                    goto step;
                }
            }
        } list_iterate_end();
        if(pid==-1){
            dbg(DBG_PRINT,"(GRADING1A)\n");
            sched_cancellable_sleep_on(&curproc->p_wait);
        }
        else {
                dbg(DBG_PRINT,"(GRADING1C)\n");             
                return -ECHILD;
            }
   } 
}


/*
 * Cancel all threads, join with them, and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */
void
do_exit(int status)
{
    dbg(DBG_PRINT,"(GRADING1C)\n");
   
    kthread_cancel(curthr,(void*)status);/*or kthread_exit*/
    curproc->p_state=PROC_DEAD;
}

size_t
proc_info(const void *arg, char *buf, size_t osize)
{
        const proc_t *p = (proc_t *) arg;
        size_t size = osize;
        proc_t *child;

        KASSERT(NULL != p);
        KASSERT(NULL != buf);

        iprintf(&buf, &size, "pid:          %i\n", p->p_pid);
        iprintf(&buf, &size, "name:         %s\n", p->p_comm);
        if (NULL != p->p_pproc) {
                iprintf(&buf, &size, "parent:       %i (%s)\n",
                        p->p_pproc->p_pid, p->p_pproc->p_comm);
        } else {
                iprintf(&buf, &size, "parent:       -\n");
        }

#ifdef __MTP__
        int count = 0;
        kthread_t *kthr;
        list_iterate_begin(&p->p_threads, kthr, kthread_t, kt_plink) {
                ++count;
        } list_iterate_end();
        iprintf(&buf, &size, "thread count: %i\n", count);
#endif

        if (list_empty(&p->p_children)) {
                iprintf(&buf, &size, "children:     -\n");
        } else {
                iprintf(&buf, &size, "children:\n");
        }
        list_iterate_begin(&p->p_children, child, proc_t, p_child_link) {
                iprintf(&buf, &size, "     %i (%s)\n", child->p_pid, child->p_comm);
        } list_iterate_end();

        iprintf(&buf, &size, "status:       %i\n", p->p_status);
        iprintf(&buf, &size, "state:        %i\n", p->p_state);

#ifdef __VFS__
#ifdef __GETCWD__
        if (NULL != p->p_cwd) {
                char cwd[256];
                lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                iprintf(&buf, &size, "cwd:          %-s\n", cwd);
        } else {
                iprintf(&buf, &size, "cwd:          -\n");
        }
#endif /* __GETCWD__ */
#endif

#ifdef __VM__
        iprintf(&buf, &size, "start brk:    0x%p\n", p->p_start_brk);
        iprintf(&buf, &size, "brk:          0x%p\n", p->p_brk);
#endif

        return size;
}

size_t
proc_list_info(const void *arg, char *buf, size_t osize)
{
        size_t size = osize;
        proc_t *p;

        KASSERT(NULL == arg);
        KASSERT(NULL != buf);

#if defined(__VFS__) && defined(__GETCWD__)
        iprintf(&buf, &size, "%5s %-13s %-18s %-s\n", "PID", "NAME", "PARENT", "CWD");
#else
        iprintf(&buf, &size, "%5s %-13s %-s\n", "PID", "NAME", "PARENT");
#endif

        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                char parent[64];
                if (NULL != p->p_pproc) {
                        snprintf(parent, sizeof(parent),
                                 "%3i (%s)", p->p_pproc->p_pid, p->p_pproc->p_comm);
                } else {
                        snprintf(parent, sizeof(parent), "  -");
                }

#if defined(__VFS__) && defined(__GETCWD__)
                if (NULL != p->p_cwd) {
                        char cwd[256];
                        lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                        iprintf(&buf, &size, " %3i  %-13s %-18s %-s\n",
                                p->p_pid, p->p_comm, parent, cwd);
                } else {
                        iprintf(&buf, &size, " %3i  %-13s %-18s -\n",
                                p->p_pid, p->p_comm, parent);
                }
#else
                iprintf(&buf, &size, " %3i  %-13s %-s\n",
                        p->p_pid, p->p_comm, parent);
#endif
        } list_iterate_end();
        return size;
}
