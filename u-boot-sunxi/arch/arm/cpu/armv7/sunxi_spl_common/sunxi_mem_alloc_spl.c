/*
 * Copyright (C) 2014 Marek Vasut <marex@denx.de>
 *
 * Command for en/de-crypting block of memory with AES-128-CBC cipher.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>

struct alloc_struct_t {
	ulong address; /* 申请内存的地址 */
	ulong size; /* 分配的内存大小，用户实际得到的内存大小 */
	ulong o_size; /* 用户申请的内存大小 */
	struct alloc_struct_t *next;
};

#define SUNXI_BYTE_ALIGN(x) (((x + 31) >> 5) << 5) /* alloc based on 1k byte   \
						      */

static struct alloc_struct_t boot_heap_head, boot_heap_tail;
/*
*********************************************************************************************************
*                       CREATE HEAP
*
* Description: create heap.
*
* Aguments   : pHeapHead    heap start address.
*              nHeapSize    heap size.
*
* Returns    : EPDK_OK/EPDK_FAIL.
*********************************************************************************************************
*/

void mem_noncache_malloc_init(uint noncache_start, uint noncache_size)
{
	boot_heap_head.size = boot_heap_tail.size = 0;
	boot_heap_head.address = noncache_start;
	boot_heap_tail.address = noncache_start + noncache_size;
	boot_heap_head.next = &boot_heap_tail;
	boot_heap_tail.next = 0;

	return;
}

/*
*********************************************************************************************************
*                       MALLOC BUFFER FROM HEAP
*
* Description: malloc a buffer from heap.
*
* Aguments   : num_bytes    the size of the buffer need malloc;
*
* Returns    : the po__s32er to buffer has malloc.
*********************************************************************************************************
*/
void *malloc_noncache(uint num_bytes)
{
	struct alloc_struct_t *ptr, *newptr;
	__u32 actual_bytes;

	if (!num_bytes)
		return NULL;

	actual_bytes = SUNXI_BYTE_ALIGN(
	    num_bytes); /* translate the byte count to size of long type */

	ptr = &boot_heap_head; /* scan from the boot_heap_head of the heap */

	/* look for enough memory for alloc */
	while (ptr && ptr->next) {
		if (ptr->next->address >=
		    (ptr->address + ptr->size +
		     2 * sizeof(struct alloc_struct_t) + actual_bytes)) {
			break;
		}
		/* find enough memory to alloc                         */
		ptr = ptr->next;
	}

	if (!ptr->next) {
		return 0; /* it has reached the boot_heap_tail of the heap now
			     */
	}

	newptr = (struct alloc_struct_t *)(ptr->address + ptr->size);
	/* create a new node for the memory block             */
	if (!newptr) {
		return 0; /* create the node failed, can't manage the block */
	}

	/* set the memory block chain, insert the node to the chain */
	newptr->address =
	    ptr->address + ptr->size + sizeof(struct alloc_struct_t);
	newptr->size = actual_bytes;
	newptr->o_size = num_bytes;
	newptr->next = ptr->next;
	ptr->next = newptr;

	return (void *)newptr->address;
}
/*
*********************************************************************************************************
*                       FREE BUFFER TO HEAP
*
* Description: free buffer to heap
*
* Aguments   : p    the po__s32er to the buffer which need be free.
*
* Returns    : none
*********************************************************************************************************
*/
void free_noncache(void *p)
{
	struct alloc_struct_t *ptr, *prev;

	if (p == NULL)
		return;

	ptr = &boot_heap_head; /* look for the node which po__s32 this memory
				  block                     */
	while (ptr && ptr->next) {
		if (ptr->next->address == (ulong)p)
			break; /* find the node which need to be release */
		ptr = ptr->next;
	}

	prev = ptr;
	ptr = ptr->next;

	if (!ptr)
		return; /* the node is heap boot_heap_tail */

	prev->next = ptr->next; /* delete the node which need be released from
				   the memory block chain  */

	return;
}

void *malloc_align(size_t size, size_t align)
{
	int *ret, *ret_align, *ret_tem;
	int tem;
	if (size <= 0 && align <= 0) {
		printf("input arg error: should bigger than 0\n");
	}
	if ((align & 0x3)) {
		printf("align arg error: align at least 4 byte.\n");
		return NULL;
	}
	size = size + align;
	ret = (int *)malloc(size);
	if (!ret) {
		printf("malloc return NULL! size = %d\n", size);
		return NULL;
	}

	if (!((int)ret & (align - 1))) {
		/* the buffer is align
		 * */
		tem = (int)ret + align;
		ret_align = (int *)(tem);
		ret_tem = ret_align;
		ret_tem--;
		*ret_tem = (int)ret;
	} else {
		tem = (int)ret & ~(align - 1);
		tem = tem + align;
		ret_align = (int *)(tem);
		ret_tem = ret_align;
		ret_tem--;
		*ret_tem = (int)ret;
	}
	return (void *)ret_align;
}

void free_align(void *ptr)
{
	int *ret;
	ret = (int *)ptr;
	ret--;
	free((void *)*ret);
}
