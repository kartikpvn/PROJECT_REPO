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
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.13 2015/12/15 14:38:24 william Exp $
 */

#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/kmalloc.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/stat.h"
#include "util/debug.h"

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read fs_op
 *      o update f_pos
 *      o fput() it
 *      o return the number of bytes read, or an error
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for reading.
 *      o EISDIR
 *        fd refers to a directory.
 *
 * In all cases, be sure you do not leak file refcounts by returning before
 * you fput() a file that you fget()'ed.
 */
int
do_read(int fd, void *buf, size_t nbytes)
{   
    /*dbg(DBG_PRINT, "Enter do_read\n");*/
        int readbytes=0;
        if(fd < 0 || fd >= 64)
        {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }

        file_t* filename=fget(fd);
        if(filename==NULL) {
            dbg(DBG_PRINT,"(GRADING2B)\n");
            return -EBADF;
        }
        if(!(filename->f_mode & FMODE_READ))
        {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                fput(filename);
                return -EBADF;
        }
        
        if(S_ISDIR(filename->f_vnode->vn_mode))
        {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                fput(filename);
                return -EISDIR;
        }

        dbg(DBG_PRINT,"(GRADING2B)\n");
        readbytes=filename->f_vnode->vn_ops->read(filename->f_vnode, filename->f_pos, buf, nbytes);
        /*filename->f_pos+=readbytes;*/
        do_lseek(fd,readbytes,SEEK_CUR);

        fput(filename);
       /* dbg(DBG_PRINT, "Exit do_read\n");*/
        return readbytes;
         
}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * fs_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int
do_write(int fd, const void *buf, size_t nbytes)
{
 /*dbg(DBG_PRINT, "Enter do_write\n");*/
        int writeBytes = 0;
        if(fd < 0 || fd >= NFILES)
        {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }

        file_t *filename=fget(fd);
        if(filename==NULL){
            dbg(DBG_PRINT,"(GRADING2B)\n");
            return -EBADF;
        }

        if((filename->f_mode&FMODE_WRITE)!= FMODE_WRITE)
        {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                fput(filename);
                return -EBADF;
        }

        /*if(S_ISDIR(filename->f_vnode->vn_mode))
        {
                dbg(DBG_TEST,"newpath4\n"); 
                fput(filename);
                return -EISDIR;

        }*/

        if((filename->f_mode&FMODE_APPEND)==FMODE_APPEND)
        {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                do_lseek(fd,0,SEEK_END);
        }
        writeBytes = filename->f_vnode->vn_ops->write(filename->f_vnode, filename->f_pos, buf, nbytes);
        if (writeBytes>=0){
                    KASSERT((S_ISCHR(filename->f_vnode->vn_mode)) ||
                            (S_ISBLK(filename->f_vnode->vn_mode)) ||
                            ((S_ISREG(filename->f_vnode->vn_mode)) && (filename->f_pos <= filename->f_vnode->vn_len)));
        dbg(DBG_PRINT,"(GRADING2A 3.a)\n");
        do_lseek(fd,writeBytes,SEEK_CUR);
        }
        fput(filename);

        return writeBytes;
    }
        

/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int
do_close(int fd)
{
    /*dbg(DBG_PRINT, "Enter do_close\n");*/

       if(fd < 0 || fd >= NFILES)
        {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }

        file_t *filename = fget(fd);
        if(filename==NULL){
            dbg(DBG_PRINT,"(GRADING2B)\n");
            return -EBADF;
        }

        dbg(DBG_PRINT,"(GRADING2B)\n");
        fput(filename);
        curproc->p_files[fd]=NULL; 
        fput(filename);
       /* dbg(DBG_PRINT, "Exit do_close\n");*/
        return 0;
        
}

/* To dup a file:
 *      o fget(fd) to up fd's refcount
 *      o get_empty_fd()
 *      o point the new fd to the same file_t* as the given fd
 *      o return the new file descriptor
 *
 * Don't fput() the fd unless something goes wrong.  Since we are creating
 * another reference to the file_t*, we want to up the refcount.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't an open file descriptor.
 *      o EMFILE
 *        The process already has the maximum number of file descriptors open
 *        and tried to open a new one.
 */
int
do_dup(int fd)
{
   /* dbg(DBG_PRINT, "Enter do_dup\n");*/
    if(fd < 0 || fd >= NFILES)
        {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }
        file_t* f=fget(fd);
        if(f==NULL){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
            }

        int fd_new=get_empty_fd(curproc);
       /* if(fd_new==-EMFILE)
        {
                dbg(DBG_TEST,"newpath3\n");
                fput(f);
                return -EMFILE;
        }*/

        dbg(DBG_PRINT,"(GRADING2B)\n");
        curproc->p_files[fd_new]=f;

/*dbg(DBG_PRINT, "Exit do_dup\n");*/
        return fd_new;
        
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
 * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
 * do_close() it first.  Then return the new file descriptor.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        ofd isn't an open file descriptor, or nfd is out of the allowed
 *        range for file descriptors.
 */
int
do_dup2(int ofd, int nfd)
{
   /* dbg(DBG_PRINT, "Enter do_dup2\n");*/
    if(ofd >= NFILES || ofd < 0){
            dbg(DBG_PRINT,"(GRADING2B)\n");
            return -EBADF;
        }
        file_t* f=fget(ofd);
        if( f==NULL){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
            }

        /*if(nfd >= NFILES || nfd < 0 )
                {
                        dbg(DBG_TEST,"newpath3\n");
                        fput(f);
                        return -EBADF;
                }*/

        if(curproc->p_files[nfd]!=NULL && ofd!=nfd)
                {
                       dbg(DBG_PRINT,"(GRADING2B)\n");
                        do_close(nfd);
                }
                if(nfd==ofd){
                    dbg(DBG_PRINT,"(GRADING2B)\n");
                    fput(f);
                    fput(f);
                    return nfd;
                }

                dbg(DBG_PRINT,"(GRADING2B)\n");
                curproc->p_files[nfd]=f;

 /*dbg(DBG_PRINT, "Exit do_dup2\n");*/
                return nfd;
               
}

/*
 * This routine creates a special file of the type specified by 'mode' at
 * the location specified by 'path'. 'mode' should be one of S_IFCHR or
 * S_IFBLK (you might note that mknod(2) normally allows one to create
 * regular files as well-- for simplicity this is not the case in Weenix).
 * 'devid', as you might expect, is the device identifier of the device
 * that the new special file should represent.
 *
 * You might use a combination of dir_namev, lookup, and the fs-specific
 * mknod (that is, the containing directory's 'mknod' vnode operation).
 * Return the result of the fs-specific mknod, or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        mode requested creation of something other than a device special
 *        file.
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mknod(const char *path, int mode, unsigned devid)
{
   /* dbg(DBG_PRINT, "Enter do_mknod\n");*/
    size_t namelen;
    const char *name;
    vnode_t *dir,*res; int result;
                        /* validate mode type */
                /*if ((S_ISCHR(mode)==0) && (S_ISBLK(mode)==0))
                {
                        dbg(DBG_TEST,"newpath1\n");
                        return -EINVAL;
                }*/
                int err;
                /*if((error = dir_namev(path, &namelen, &name, NULL, &dir)) != 0)
                {
                        dbg(DBG_TEST,"newpath2\n");
                        return error;
                    }*/
                dir_namev(path, &namelen, &name, NULL, &dir);
                err=lookup(dir, name, namelen, &res);
                if(err != 0){
                    if (err == -ENOENT)
                        {
                                KASSERT(NULL != dir->vn_ops->mknod);
                                dbg(DBG_PRINT,"(GRADING2A 3.b)\n");
                                result= dir->vn_ops->mknod(dir, name, namelen, mode, (devid_t)devid);
                                vput(dir);
                                return result;
                        }
                }
                return 0;
                /*if((err=lookup(dir, name, namelen, &res)) == 0){
                        dbg(DBG_TEST,"newpath3\n");
                        vput(dir);
                        vput(res);    
                        return -EEXIST;
                    }
                
                else{
                        if (err == -ENOTDIR)
                        {
                                dbg(DBG_TEST,"newpath4\n");
                                vput(dir);
                                return -ENOTDIR;
                        }
                        
                        if (err == -ENOENT)
                        {
                                KASSERT(NULL != dir->vn_ops->mknod);
                                dbg(DBG_PRINT,"(GRADING2A 3.b)\n");
                                result= dir->vn_ops->mknod(dir, name, namelen, mode, (devid_t)devid);
                                vput(dir);
                                return result;
                        }
                        dbg(DBG_TEST,"newpath5\n");
                        vput(dir);
                        return err;
                    }*/
                              
}

/* Use dir_namev() to find the vnode of the dir we want to make the new
 * directory in.  Then use lookup() to make sure it doesn't already exist.
 * Finally call the dir's mkdir vn_ops. Return what it returns.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mkdir(const char *path)
{
     /*dbg(DBG_PRINT, "Enter do_mkdir\n");*/
        vnode_t *dir,*res_vnode;
        size_t namelen;
        const char *name;
        int error,error1,res;
        /* Err including, ENOENT, ENOTDIR, ENAMETOOLONG*/
        if((error = dir_namev(path, &namelen, &name, NULL, &dir) ) != 0) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return error;
        }

        if(namelen==0) {
            dbg(DBG_PRINT,"(GRADING2B)\n");
            vput(dir);
            return -EEXIST;
        }


        if((error1 = lookup(dir, name, namelen, &res_vnode)) == 0) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                vput(dir);
                vput(res_vnode);
                return -EEXIST;
        }
 
        if(error1 == -ENOTDIR){
            dbg(DBG_PRINT,"(GRADING2B)\n");
            vput(dir);
            return error1;
        }

        KASSERT(NULL != dir->vn_ops->mkdir);
        dbg(DBG_PRINT,"(GRADING2A 3.c)\n");
        res= dir->vn_ops->mkdir(dir, name, namelen);
        vput(dir);
        return res;
   
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
 * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
 * return an error if the dir to be removed does not exist or is not empty, so
 * you don't need to worry about that here. Return the value of the v_op,
 * or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        path has "." as its final component.
 *      o ENOTEMPTY
 *        path has ".." as its final component.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_rmdir(const char *path)
{
        /*dbg(DBG_PRINT, "Enter do_rmdir\n");*/
        int error,error1,res;
        vnode_t *dir, *res_vnode;
        size_t dirlength;
        const char *dirname;

        /*dir_namev should check ENAMETOOLONG and ENOENT, if this happens it will return that code*/
        if((error = dir_namev(path, &dirlength, &dirname, NULL, &dir))!= 0) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return error;
        }

       if((error1 = lookup(dir, dirname, dirlength, &res_vnode)) != 0) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                vput(dir);
                return error1;
        }
        /*Error situation to be handled*/
        if(name_match(".",dirname,dirlength)) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                vput(dir);
                vput(res_vnode);
                return -EINVAL;
        } else if(name_match("..",dirname,dirlength)) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                vput(dir);
                vput(res_vnode);
                return -ENOTEMPTY;
        } 
        
        KASSERT(NULL != dir->vn_ops->rmdir);
        dbg(DBG_PRINT,"(GRADING2A 3.d)\n");
        dbg(DBG_PRINT,"(GRADING2B)\n");
        res = dir->vn_ops->rmdir(dir, dirname, dirlength);
        vput(dir);
        vput(res_vnode);
       /* dbg(DBG_PRINT, "Exit do_rmdir\n");*/
        return res;
      
}

/*
 * Same as do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EISDIR
 *        path refers to a directory.
 *      o ENOENT
 *        A component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_unlink(const char *path)
{
    /*dbg(DBG_PRINT, "Enter do_unlink\n");*/
        int err,errno,res;
        vnode_t *file, *dir;
        size_t fileNameLength;
        const char *filename;

        /*dev_namev should check ENAMETOOLONG  and ENOENT, if this happens it will return that code*/
       /* if((err = dir_namev(path, &fileNameLength, &filename, NULL, &dir))!= 0) {
                dbg(DBG_TEST,"newpath1\n");
                return err;
        }*/
        dir_namev(path, &fileNameLength, &filename, NULL, &dir);
        
        if ((errno = lookup(dir, filename, fileNameLength, &file)) != 0) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                vput(dir);
                return  errno;
        }

        if(S_ISDIR(file->vn_mode)) {
            dbg(DBG_PRINT,"(GRADING2B)\n");
            vput(dir);
            vput(file);
            return -EISDIR;
        }
        
        KASSERT(NULL != dir->vn_ops->unlink);
        dbg(DBG_PRINT,"(GRADING2A 3.e)\n");
        dbg(DBG_PRINT,"(GRADING2B)\n");
        res = dir->vn_ops->unlink(dir, filename, fileNameLength);

        vput(file);
        vput(dir);
/*dbg(DBG_PRINT, "Exit do_unlink\n");*/
        return res;
}

/* To link:
 *      o open_namev(from)
 *      o dir_namev(to)
 *      o call the destination dir's (to) link vn_ops.
 *      o return the result of link, or an error
 *
 * Remember to vput the vnodes returned from open_namev and dir_namev.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        to already exists.
 *      o ENOENT
 *        A directory component in from or to does not exist.
 *      o ENOTDIR
 *        A component used as a directory in from or to is not, in fact, a
 *        directory.
 *      o ENAMETOOLONG
 *        A component of from or to was too long.
 *      o EISDIR
 *        from is a directory.
 */
int
do_link(const char *from, const char *to)
{
   /* dbg(DBG_PRINT, "Enter do_link\n");
        vnode_t *from_vnode, *to_vnode, *to_vnode1;
        int errno;
        size_t namelen;
        const char *name;

        if((errno = open_namev(from, 0, &from_vnode, NULL))!=0) {                                                                                                  
                return errno;
        }
        if(S_ISDIR(from_vnode->vn_mode)) {
            vput(from_vnode);
           return -EISDIR;
       }*/

        /*ENOENT, ENOTDIR, ENAMETOOLONG, EISDIR will be returned from this function*/
       /* if((errno = dir_namev(to, &namelen, &name, NULL, &to_vnode)!=0)) {
                vput(from_vnode);
                return errno;
        }
       
        if(lookup(to_vnode, name, namelen, &to_vnode1) == 0) {
                vput(from_vnode);
                vput(to_vnode);
                vput(to_vnode1);
                return -EEXIST;
        }
        if(!S_ISDIR(to_vnode->vn_mode)) {
            vput(from_vnode);
            vput(to_vnode);
            return -ENOTDIR;
        }
        
        int ret = to_vnode->vn_ops->link(from_vnode, to_vnode, name, namelen);
        vput(from_vnode);
        vput(to_vnode);
        dbg(DBG_PRINT, "Exit do_link\n");
        return ret;*/
    NOT_YET_IMPLEMENTED("VFS: do_link");
    return -1;
       
}

/*      o link newname to oldname
 *      o unlink oldname
 *      o return the value of unlink, or an error
 *
 * Note that this does not provide the same behavior as the
 * Linux system call (if unlink fails then two links to the
 * file could exist).
 */
int
do_rename(const char *oldname, const char *newname)
{
        /*dbg(DBG_PRINT, "Enter do_rename\n");
        int ret = do_link(oldname, newname);
       
        if(ret!=0){
                return ret;
            }
        else {
                ret = do_unlink(oldname);
        }
        dbg(DBG_PRINT, "Exit do_rename\n");
        return ret;*/
    NOT_YET_IMPLEMENTED("VFS: do_rename");
    return -1;
        
}

/* Make the named directory the current process's cwd (current working
 * directory).  Don't forget to down the refcount to the old cwd (vput()) and
 * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
 * success.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        path does not exist.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o ENOTDIR
 *        A component of path is not a directory.
 */
int
do_chdir(const char *path)
{
    /*dbg(DBG_PRINT, "Enter do_chdir\n");*/
        int res;
        vnode_t *o_cwd, *n_cwd;
        o_cwd=curproc->p_cwd;

        if((res = open_namev(path,0, &n_cwd,NULL))!=0){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return res;
                }

        if(!S_ISDIR(n_cwd->vn_mode)){
            dbg(DBG_PRINT,"(GRADING2B)\n");
            vput(n_cwd);
            return -ENOTDIR;
            }
        dbg(DBG_PRINT,"(GRADING2B)\n");
        curproc->p_cwd =  n_cwd;
        vput(o_cwd);
          /*dbg(DBG_PRINT, "Exit do_chdir\n");*/
        return 0;
        /*NOT_YET_IMPLEMENTED("VFS: do_chdir");
        return -1;*/
      
}

/* Call the readdir fs_op on the given fd, filling in the given dirent_t*.
 * If the readdir fs_op is successful, it will return a positive value which
 * is the number of bytes copied to the dirent_t.  You need to increment the
 * file_t's f_pos by this amount.  As always, be aware of refcounts, check
 * the return value of the fget and the virtual function, and be sure the
 * virtual function exists (is not null) before calling it.
 *
 * Return either 0 or sizeof(dirent_t), or -errno.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        Invalid file descriptor fd.
 *      o ENOTDIR
 *        File descriptor does not refer to a directory.
 */
int
do_getdent(int fd, struct dirent *dirp)
{
    /*dbg(DBG_PRINT, "Enter do_getdent\n");*/
        if(fd < 0 || fd >= NFILES){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
            }
        file_t *file= fget(fd);
        if(file==NULL) {
            dbg(DBG_PRINT,"(GRADING2B)\n");
            return -EBADF;
        }

        if(!S_ISDIR(file->f_vnode->vn_mode)) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                fput(file);
                return -ENOTDIR;   
                }    

        int dir_bytes;
        dir_bytes =file->f_vnode->vn_ops->readdir(file->f_vnode,file->f_pos, dirp);
        if(dir_bytes==0){
            dbg(DBG_PRINT,"(GRADING2B)\n");
            (void)do_lseek(fd,dir_bytes,SEEK_END);
            fput(file);
            return 0;
        }
        if(dir_bytes){ 
                dbg(DBG_PRINT,"(GRADING2B)\n");   
                (void)do_lseek(fd,dir_bytes,SEEK_CUR);
                fput(file);
                }
        dbg(DBG_PRINT,"(GRADING2B)\n");
        return sizeof(*dirp);
       /* NOT_YET_IMPLEMENTED("VFS: do_getdent");
        return -1;*/
      
}

/*
 * Modify f_pos according to offset and whence.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not an open file descriptor.
 *      o EINVAL
 *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
 *        file offset would be negative.
 */
int
do_lseek(int fd, int offset, int whence)
{
    /*dbg(DBG_PRINT, "Enter do_lseek\n");*/
    if(fd < 0 || fd >= NFILES)
        {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }

        file_t *file= fget(fd);
        if(file==NULL) {
            dbg(DBG_PRINT,"(GRADING2B)\n");
            return -EBADF;
        }
        if(whence!=SEEK_SET && whence!=SEEK_CUR && whence!=SEEK_END) {
            dbg(DBG_PRINT,"(GRADING2B)\n");
            fput(file);
            return -EINVAL;
        }

        /* Modilfy the f_pos */
        if(whence==SEEK_SET) {         /*SEEK_SET*/
                if(offset<0) { 
                        dbg(DBG_PRINT,"(GRADING2B)\n"); 
                        fput(file);
                        return -EINVAL;
                        }
                dbg(DBG_PRINT,"(GRADING2B)\n");
                file->f_pos = offset;
                }
        else if(whence==SEEK_CUR){  /*SEEK_CUR*/
                if(file->f_pos+offset < 0) {
                        dbg(DBG_PRINT,"(GRADING2B)\n");  
                        fput(file);
                        return -EINVAL;
                        }
                dbg(DBG_PRINT,"(GRADING2B)\n");
                file->f_pos = file->f_pos+offset;
                }
        else if(whence==SEEK_END){  /*SEEK_END*/
                if(file->f_vnode->vn_len+offset<0) { 
                        dbg(DBG_PRINT,"(GRADING2B)\n"); 
                        fput(file);
                        return -EINVAL;
                        } 
                dbg(DBG_PRINT,"(GRADING2B)\n");    
                file->f_pos = file->f_vnode->vn_len+offset;
                }
        fput(file);
        /*dbg(DBG_PRINT, "Exit do_lseek\n");*/
        return file->f_pos;
         
        /*NOT_YET_IMPLEMENTED("VFS: do_lseek");
        return -1;*/
}

/*
 * Find the vnode associated with the path, and call the stat() vnode operation.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        A component of path does not exist.
 *      o ENOTDIR
 *        A component of the path prefix of path is not a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_stat(const char *path, struct stat *buf)
{
      /*dbg(DBG_PRINT, "Enter do_stat\n");*/
        vnode_t *path_vnode; 
        int res;
        if((res = open_namev(path,0, &path_vnode,NULL))!=0) {
            dbg(DBG_PRINT,"(GRADING2B)\n");
            return res;
        }
        KASSERT(NULL != path_vnode->vn_ops->stat);
        dbg(DBG_PRINT,"(GRADING2A 3.f)\n");
        dbg(DBG_PRINT,"(GRADING2B)\n");
        path_vnode->vn_ops->stat(path_vnode,buf);
       /* dbg(DBG_PRINT, "Exit do_stat\n");*/
        vput(path_vnode);
        return 0;
}


#ifdef __MOUNTING__
/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutely sure your Weenix is perfect.
 *
 * This is the syscall entry point into vfs for mounting. You will need to
 * create the fs_t struct and populate its fs_dev and fs_type fields before
 * calling vfs's mountfunc(). mountfunc() will use the fields you populated
 * in order to determine which underlying filesystem's mount function should
 * be run, then it will finish setting up the fs_t struct. At this point you
 * have a fully functioning file system, however it is not mounted on the
 * virtual file system, you will need to call vfs_mount to do this.
 *
 * There are lots of things which can go wrong here. Make sure you have good
 * error handling. Remember the fs_dev and fs_type buffers have limited size
 * so you should not write arbitrary length strings to them.
 */
int
do_mount(const char *source, const char *target, const char *type)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
        return -EINVAL;
}

/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutley sure your Weenix is perfect.
 *
 * This function delegates all of the real work to vfs_umount. You should not worry
 * about freeing the fs_t struct here, that is done in vfs_umount. All this function
 * does is figure out which file system to pass to vfs_umount and do good error
 * checking.
 */
int
do_umount(const char *target)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
        return -EINVAL;
}
#endif
