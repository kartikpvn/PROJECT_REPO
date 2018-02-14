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
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
    dbg(DBG_TEST,"enter vmap_create\n");
        vmmap_t* vm_map = (vmmap_t*)slab_obj_alloc(vmmap_allocator);
        if(vm_map==NULL)
            return NULL;
        memset(vm_map,0,sizeof(vmmap_t*));

        list_init(&vm_map->vmm_list);  
        vm_map->vmm_proc = NULL;
        dbg(DBG_TEST,"exit vmmap create\n");
        return vm_map;
        /*NOT_YET_IMPLEMENTED("VM: vmmap_create");
        return NULL;*/
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
     dbg(DBG_TEST,"enter vmmap_destroy\n");
    KASSERT(NULL != map);
    vmarea_t* vm_area;
       list_iterate_begin(&map->vmm_list,vm_area,vmarea_t,vma_plink){
            list_remove(&vm_area->vma_plink);
            vmarea_free(vm_area);
       }list_iterate_end();
       if(list_empty(&map->vmm_list)){
            map->vmm_proc=NULL;
            slab_obj_free(vmmap_allocator,map);
       }
        dbg(DBG_TEST,"exit vmmap_destroy\n");
        /*NOT_YET_IMPLEMENTED("VM: vmmap_destroy");*/
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
     dbg(DBG_TEST,"enter vmmap_insert\n");
    KASSERT(NULL != map && NULL != newvma);
    KASSERT(NULL == newvma->vma_vmmap);
    KASSERT(newvma->vma_start < newvma->vma_end);
    KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);

    vmarea_t* vm_area;
        list_iterate_begin(&map->vmm_list,vm_area,vmarea_t,vma_plink){
                if(vm_area->vma_start > newvma->vma_start){
                    list_insert_before(&vm_area->vma_plink,&newvma->vma_plink);
                    vm_area->vma_vmmap = map;
                    return;
                }
            }list_iterate_end();
        list_insert_tail(&map->vmm_list,&newvma->vma_plink);
        vm_area->vma_vmmap = map;
         dbg(DBG_TEST,"exit vmmap_insert\n");
       /* NOT_YET_IMPLEMENTED("VM: vmmap_insert");*/
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
     dbg(DBG_TEST,"enter vmmap_find_range\n");
    uint32_t low_addr = ADDR_TO_PN(USER_MEM_LOW);
    uint32_t high_addr = ADDR_TO_PN(USER_MEM_HIGH);

    uint32_t vfn;
    vmarea_t* vm_area;
    if(dir == VMMAP_DIR_HILO){
        list_iterate_reverse(&map->vmm_list,vm_area,vmarea_t,vma_plink){
            if((high_addr-vm_area->vma_end) >= npages){
                vfn = high_addr-npages;
                return vfn;
                }
            else
                high_addr = vm_area->vma_start;
        }list_iterate_end();

        if((high_addr - low_addr)>= npages){
            vfn = high_addr - npages;
            return vfn;
        }
    }

    else if(dir == VMMAP_DIR_LOHI){
        list_iterate_begin(&map->vmm_list,vm_area,vmarea_t,vma_plink){
                if((vm_area->vma_start-low_addr) >= npages){
                    vfn = low_addr;
                    return vfn;
                    } 
                else
                    low_addr=vm_area->vma_end;  
        }list_iterate_end();
    
    if((high_addr - low_addr) >= npages){
            vfn = low_addr;
            return vfn;
            }
    }
     dbg(DBG_TEST,"exit vmmap_find_range\n");
    return -1;

        /*NOT_YET_IMPLEMENTED("VM: vmmap_find_range");
        return -1;*/
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
     dbg(DBG_TEST,"enter vmmap_lookup\n");
    KASSERT(NULL != map);
        vmarea_t* vm_area;
        list_iterate_begin(&map->vmm_list,vm_area,vmarea_t,vma_plink){
            if ((vm_area->vma_start <= vfn) && (vm_area->vma_end > vfn)){
                    return vm_area;
                }
        }list_iterate_end();
        /*NOT_YET_IMPLEMENTED("VM: vmmap_lookup");*/
        dbg(DBG_TEST,"exit vmmap_lookup\n");
        return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
    dbg(DBG_TEST,"enter vmmap_clone\n");
       vmmap_t *new_map = vmmap_create();
       if(new_map==NULL)    
            return NULL;
       vmarea_t* vm_area;
       vmarea_t* new_vma;
       list_iterate_begin(&map->vmm_list,vm_area,vmarea_t,vma_plink){
            new_vma = vmarea_alloc();
            if(new_vma==NULL)
                return NULL;
            new_vma->vma_start = vm_area->vma_start;
            new_vma->vma_end = vm_area->vma_end;
            vmmap_insert(new_map, new_vma);
            new_vma->vma_prot = vm_area->vma_prot;
            new_vma->vma_obj=NULL;
       }list_iterate_end();
       dbg(DBG_TEST,"exit vmmap_clone\n");
       return new_map;
        /*NOT_YET_IMPLEMENTED("VM: vmmap_clone");
        return NULL;*/
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{
    dbg(DBG_TEST,"enter vmmap_map\n");
    KASSERT(NULL != map);
    KASSERT(0 < npages);
    KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
    KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
    KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
    KASSERT(PAGE_ALIGNED(off));
    int vfn,res;
    mmobj_t *file_obj;
    vmarea_t* vma;
    if(lopage==0){
            vfn = vmmap_find_range(map,npages,dir);
            if(vfn ==-1){
               return -1;
            }
            lopage=vfn;
    }else{
            if (vmmap_is_range_empty(map,lopage,npages) == 0){
                res = vmmap_remove(map,lopage,npages); 
                if(res!=0){
                    return res;
                   }
                }
                /**new = vmarea_alloc();
                *new->vma_start= lopage;
                *new->new_end = lopage+npages;*/
                
        }
        vma = vmarea_alloc();
        vma->vma_start= lopage;
        vma->vma_end = lopage+npages;
        vma->vma_prot = prot;
        vma->vma_flags = flags;
        vma->vma_off= off;

    if(file==NULL){
        file_obj = anon_create();
    }
    else{
        res = file->vn_ops->mmap(file,vma,&file_obj);
        if(res!=0){
            return res;
        }
        file->vn_mmobj = *file_obj;
    }
    vma->vma_obj=file_obj;

    if(flags&MAP_PRIVATE){
        file_obj =shadow_create();
    }
    vmmap_insert(map,vma);

    if(new!=NULL){
        vma->vma_obj->mmo_refcount++;
        new=&vma;
    }
    /*else{
        dbg(DBG_TEST,"exit vmmap_map\n");
        return -1;
    }*/

    dbg(DBG_TEST,"exit vmmap_map\n");
    
    return 0;
        /*NOT_YET_IMPLEMENTED("VM: vmmap_map");
        return -1;*/
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped wi
 ll play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
    dbg(DBG_TEST,"enter vmmap_remove\n");
    vmarea_t* vm_area;
    if(!list_empty(&map->vmm_list)) {
        list_iterate_begin(&map->vmm_list,vm_area,vmarea_t,vma_plink){ 
            
            if(vm_area->vma_start < lopage && vm_area->vma_end > lopage) {
                /*case1*/
                vmarea_t* vma1=vmarea_alloc();
                vmarea_t* vma2=vmarea_alloc();

                vma1->vma_start = vm_area->vma_start;
                vma1->vma_end = lopage;

                vma2->vma_start = lopage+npages;
                vma2->vma_end = vm_area->vma_end;

                vma1->vma_off = vm_area->vma_off;

                vma2->vma_off = vm_area->vma_off + (lopage+npages) - vm_area->vma_start;

                vma1->vma_prot = vma2->vma_prot = vm_area->vma_prot;
                vma1->vma_flags = vma2->vma_flags = vm_area->vma_flags;
                vma1->vma_obj = vma2->vma_obj = vm_area->vma_obj;
                list_remove(&vm_area->vma_plink);
                /*Do we need to free the vmarea here*/
                vmmap_insert(map, vma1);
                vmmap_insert(map, vma2);
                vma1->vma_obj->mmo_refcount++;
           /* how to increment the
      reference count to the file associated with the vmarea.*/    

            }
            else if((vm_area->vma_start < lopage) && vm_area->vma_end > lopage &&(vm_area->vma_end <= lopage+npages)){
                /*case2*/
                vm_area->vma_end = lopage+npages;
            }
            else if((vm_area->vma_start < (lopage+npages))&& (vm_area->vma_end >= lopage+npages)){
                /*case3*/
                vm_area->vma_start = lopage+npages;
                vm_area->vma_off = vm_area->vma_off + (lopage+npages) - vm_area->vma_start; /*verify*/
            }
            else if(vm_area->vma_start >=lopage && vm_area->vma_end <=lopage+npages){
                /*case4*/
                list_remove(&vm_area->vma_plink);
                vmarea_free(vm_area);
            }
        }list_iterate_end();
    }
    dbg(DBG_TEST,"exit vmmap_remove\n");
    return 0;    
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
    dbg(DBG_TEST,"enter vmmap_range\n");
    uint32_t endvfn = startvfn+npages;
    KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
    vmarea_t* vm_area;
    list_iterate_begin(&map->vmm_list,vm_area,vmarea_t,vma_plink){
        /*2 cases when vma is completely inide req range half of it is inside req range*/
        if(startvfn < vm_area->vma_end  && vm_area->vma_end <= endvfn){
            return 0;
        }

        if(vm_area->vma_end >= endvfn){
            if(vm_area->vma_start <= startvfn)
                return 0;
            else if(vm_area->vma_start > startvfn && vm_area->vma_start < endvfn)
                return 0;
        }
    }list_iterate_end();
    dbg(DBG_TEST,"exit vmmap_range\n");
    return 1;
        /*NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");
        return 0;*/
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
    dbg(DBG_TEST,"enter vmmap_read\n");
    uint32_t vfn = ADDR_TO_PN(vaddr);
    uint32_t addr = (uint32_t)vaddr;
    pframe_t *pf; int size;
    size_t readbytes =0;
    /*loop*/
   
        while(readbytes<count){
        vmarea_t* vma = vmmap_lookup(map,vfn);
        if(vma==NULL)
            return -EFAULT;/*not sure which errno*/

        int pagenum= vfn - vma->vma_start + vma->vma_off;
        int res = pframe_lookup(vma->vma_obj,pagenum,0, &pf);
        if(res!=0)
            return res;
        
            uint32_t off = addr % PAGE_SIZE;
            size = (PAGE_SIZE-off)-(count-readbytes)>0 ?(count-readbytes) : (PAGE_SIZE-off);
            memcpy((char*)buf,(char*)pf->pf_addr+off,size);
            addr = addr+size;
            readbytes = readbytes +size;
            vfn++;

        }
    dbg(DBG_TEST,"exit vmmap_lookup\n");

    return 0;

        /*NOT_YET_IMPLEMENTED("VM: vmmap_read");
        return 0;*/
}


/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
    dbg(DBG_TEST,"enter vmmap_write\n");
    uint32_t vfn = ADDR_TO_PN(vaddr);
    uint32_t addr = (uint32_t)vaddr;
    pframe_t *pf; int size;
    size_t writebytes =0;

int pagenum;
    /*loop*/
   
        while(writebytes<count){
    
        vmarea_t* vma = vmmap_lookup(map,vfn);
        if(vma==NULL){
            dbg(DBG_TEST,"exit vmmap_write1\n");
            return -EFAULT; /*not sure which errno*/
        }
    vfn = ADDR_TO_PN(vaddr);
        pagenum = vfn - vma->vma_start + vma->vma_off;
        /*int res = pframe_lookup(vma->vma_obj,pagenum,1, &pf);*/
        int res = pframe_lookup(vma->vma_obj,pagenum,1, &pf);
        if(res!=0){
            dbg(DBG_TEST,"exit vmmap_write2\n");
            return res;
        }
            int ret = pframe_dirty(pf);
            if(ret!=0){
                return ret;
            }
            uint32_t off = addr % PAGE_SIZE;
            size = (PAGE_SIZE-off)-(count-writebytes)>0 ?(count-writebytes) : (PAGE_SIZE-off);
            memcpy((char*)pf->pf_addr+off,(char*)buf+writebytes,size);
            addr = addr+size;
            writebytes = writebytes +size;
            vfn++;

        }
        dbg(DBG_TEST,"exit vmmap_write\n");
        return 0;
        /*NOT_YET_IMPLEMENTED("VM: vmmap_write");
        return 0;*/
}