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

/*
 *  FILE: open.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Mon Apr  6 19:27:49 1998
 */

#include "globals.h"
#include "errno.h"
#include "fs/fcntl.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/stat.h"
#include "util/debug.h"

/* find empty index in p->p_files[] */
int
get_empty_fd(proc_t *p)
{
        int fd;

        for (fd = 0; fd < NFILES; fd++) {
                if (!p->p_files[fd])
                        return fd;
        }

        dbg(DBG_ERROR | DBG_VFS, "ERROR: get_empty_fd: out of file descriptors "
            "for pid %d\n", curproc->p_pid);
        return -EMFILE;
}

/*
 * There a number of steps to opening a file:
 *      1. Get the next empty file descriptor.
 *      2. Call fget to get a fresh file_t.
 *      3. Save the file_t in curproc's file descriptor table.
 *      4. Set file_t->f_mode to OR of FMODE_(READ|WRITE|APPEND) based on
 *         oflags, which can be O_RDONLY, O_WRONLY or O_RDWR, possibly OR'd with
 *         O_APPEND or O_CREAT.
 *      5. Use open_namev() to get the vnode for the file_t.
 *      6. Fill in the fields of the file_t.
 *      7. Return new fd.
 *
 * If anything goes wrong at any point (specifically if the call to open_namev
 * fails), be sure to remove the fd from curproc, fput the file_t and return an
 * error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        oflags is not valid.
 *      o EMFILE
 *        The process already has the maximum number of files open.
 *      o ENOMEM
 *        Insufficient kernel memory was available.
 *      o ENAMETOOLONG
 *        A component of filename was too long.
 *      o ENOENT
 *        O_CREAT is not set and the named file does not exist.  Or, a
 *        directory component in pathname does not exist.
 *      o EISDIR
 *        pathname refers to a directory and the access requested involved
 *        writing (that is, O_WRONLY or O_RDWR is set).
 *      o ENXIO
 *        pathname refers to a device special file and no corresponding device
 *        exists.
 */

int
do_open(const char *filename, int oflags)
{
   /* dbg(DBG_PRINT, "Enter do_open\n");*/
    int flag=0,flag1=0,flag_create=0,flagbit=0;

    int fd = get_empty_fd(curproc);
    /*if(fd==-EMFILE) {
        dbg(DBG_TEST,"newpath1\n");
        return -EMFILE;
    }*/

    file_t* file_new = fget(-1);
    /*if(file_new==NULL) {
        dbg(DBG_TEST,"newpath2\n");
        return -ENOMEM;
    }*/

    curproc->p_files[fd] = file_new;
    vnode_t *res_vnode;

/*Check the status of flags*/
    if((oflags&O_RDWR) == O_RDWR) {
        if((oflags&O_WRONLY) == O_WRONLY) {
            dbg(DBG_PRINT,"(GRADING2B)\n");
            curproc->p_files[fd] = NULL;
            fput(file_new);
            return -EINVAL;
        } else {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                flag = (FMODE_READ | FMODE_WRITE);
        }
    } else if((oflags&O_WRONLY) == O_WRONLY) {
        dbg(DBG_PRINT,"(GRADING2B)\n");
       flag = FMODE_WRITE; 
    } else if((oflags&O_RDONLY) == O_RDONLY) {
        dbg(DBG_PRINT,"(GRADING2B)\n");
       flag = FMODE_READ;  
    }
    if(!flag){
            dbg(DBG_TEST,"newpath7\n");
            curproc->p_files[fd] = NULL;
            fput(file_new);
            return -EINVAL;
    }

    if((oflags&O_APPEND) == O_APPEND)  {
            dbg(DBG_PRINT,"(GRADING2B)\n");
           flag1 = FMODE_APPEND;
       }

    /*Update file data fields*/
    file_new->f_mode= (flag|flag1);
    file_new->f_pos=0;

    if(filename[strlen(filename)-1] == '/'){
        dbg(DBG_PRINT,"(GRADING2B)\n");
        flagbit=1;
    }

/*Check create flag and call open_namev with that*/
    if((oflags&O_CREAT) == O_CREAT)  {
            dbg(DBG_PRINT,"(GRADING2B)\n");
            flag_create = O_CREAT;
        }

    int error = open_namev(filename,flag_create,&res_vnode,NULL);
    if(error!=0){
            dbg(DBG_PRINT,"(GRADING2B)\n");
            curproc->p_files[fd] = NULL;
            fput(file_new);
            return error;
    }

    if(flagbit==1 && (!S_ISDIR(res_vnode->vn_mode))){
        dbg(DBG_PRINT,"(GRADING2B)\n");
        curproc->p_files[fd] = NULL;
        fput(file_new);
        vput(res_vnode);
        return -ENOTDIR;
    }

    if(S_ISDIR(res_vnode->vn_mode) && ((flag == FMODE_WRITE) | (flag ==(FMODE_READ | FMODE_WRITE)))){
        dbg(DBG_PRINT,"(GRADING2B)\n");
        curproc->p_files[fd] = NULL;
        fput(file_new);
        vput(res_vnode);
        return -EISDIR;
    }

    if(S_ISBLK(res_vnode->vn_mode)){
        dbg(DBG_TEST,"newpath14\n");
        blockdev_t *blk = blockdev_lookup(res_vnode->vn_devid);
        if(blk==NULL){
            dbg(DBG_TEST,"newpath15\n");
            curproc->p_files[fd] = NULL;
            fput(file_new);
            vput(res_vnode);
            return -ENXIO;
        }
    }
    if(S_ISCHR(res_vnode->vn_mode)){
        dbg(DBG_PRINT,"(GRADING2B)\n");
        bytedev_t *btdev = bytedev_lookup(res_vnode->vn_devid);
        /*if(btdev==NULL)  {
            dbg(DBG_TEST,"newpath17\n");
            curproc->p_files[fd] = NULL;
            fput(file_new);
            vput(res_vnode);
            return -ENXIO;
        }*/
    }
    
    dbg(DBG_PRINT,"(GRADING2B)\n");
    file_new->f_vnode= res_vnode;
    /*dbg(DBG_PRINT, "Exit do_open\n");*/
    return fd;
     
}
