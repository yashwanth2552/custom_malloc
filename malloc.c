#include "malloc.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define HEADER_SIZE sizeof(header)
#define SPLITTING_THRESHOLD (HEADER_SIZE+1)
#define MAGIC 0x4D414C43

void* heap_start_ptr = NULL;
void* first_free_ptr = NULL;
void* last_alloc_ptr = NULL;

void heap_dump() {
    header* cur = (header*)heap_start_ptr;
    printf("Heap dump start\n");
    while (cur != NULL) {
        printf("is_free:%d, prev:%p, next:%p, size:%d, magic:0x%X\n", cur->is_free, cur->prev, cur->next, cur->size, cur->magic);
        cur = (header*)(cur->next);
    }
    printf("Heap dump end\n");
}

bool valid_free_ptr(void* ptr) {
    header* ptr_header = (header*)((char*)ptr - HEADER_SIZE);
    return (ptr_header->magic == MAGIC);
}

void update_first_free_ptr_after_unsplit_alloc(void* cur) {
    if (first_free_ptr < cur) return ;
    void* tmp_cur = first_free_ptr;
    while (tmp_cur && !((header*)tmp_cur)->is_free) {
        tmp_cur = ((header*)tmp_cur)->next;
    }
    first_free_ptr = tmp_cur;
}

void update_first_free_ptr_after_split_alloc(void* cur, void* new_empty_block) {
    if (first_free_ptr == cur) first_free_ptr = new_empty_block;
}

void* custom_malloc(int act_req_size) {
    int req_size = (act_req_size/8)*8;
    if (act_req_size % 8 !=0) {
        req_size += 8;
    }
    /*
     1)We need to start checking from first_free_ptr to see if there is a free block of size >= req_size
       a)If first_free_ptr == NULL, it means either this is the first alloaction or there is not free block at all.
       b)In either case we need to ask OS via sbrk() to extend heap boundary.
     */
    void* prev = last_alloc_ptr;
    void* cur = first_free_ptr;
    while (cur && (!((header*)cur)->is_free ||((header*)cur)->size < req_size)) {
        prev = cur;
        cur = ((header*)cur)->next;
    }
    //No freed block of atleast requested size exists.
    if (cur == NULL) {
        void* new_brk = sbrk(HEADER_SIZE + req_size);
        header* new_header = (header*)new_brk;
        new_header->prev = prev;
        new_header->next = NULL;
        new_header->is_free = false;
        new_header->magic = MAGIC;
        new_header->size = req_size;
        if (prev) {
            ((header*)prev)->next = new_brk;
        }
        if(last_alloc_ptr == NULL)heap_start_ptr = new_brk;
        last_alloc_ptr = new_brk;
        return (char*)new_brk + HEADER_SIZE;
    }

    //Freed block of atleast requested size exists!!
    /*
     1)To split, we need to check if splitting threshold is met.
     2)If not met, simple!!
     3)If met, then we need to split.
     */
    if (((header*)cur)->size - req_size < SPLITTING_THRESHOLD) {
        ((header*)cur)->is_free = false;
        update_first_free_ptr_after_unsplit_alloc(cur);
        return (char*)cur + HEADER_SIZE;
    }

    void* new_empty_block = (char*)cur + HEADER_SIZE + req_size;
    header* new_empty_header = (header*)(new_empty_block);
    new_empty_header->is_free = true;
    new_empty_header->magic = MAGIC;
    new_empty_header->size = ((header*)cur)->size - req_size - HEADER_SIZE;
    new_empty_header->prev = cur;
    new_empty_header->next = ((header*)cur)->next;
    ((header*)cur)->next = new_empty_block;
    ((header*)cur)->is_free = false;
    ((header*)cur)->size = req_size;
    update_first_free_ptr_after_split_alloc(cur, new_empty_block);
    if (last_alloc_ptr == cur) last_alloc_ptr = new_empty_block;
    return (char*)cur + HEADER_SIZE;
}

bool custom_free(void* ptr) {
    //check if ptr is valid
    if (!valid_free_ptr(ptr)) {
        printf("Invalid ptr passed to custom_free\n");
        return false;
    }
    header* ptr_header = (header*)((char*)ptr - HEADER_SIZE);
    ptr_header->is_free = true;
    /*
     1)check if we can merge with previous block.
     */
    header* prev_header = (header*)(ptr_header->prev);
    if (prev_header && prev_header->is_free) {
        prev_header->size += (HEADER_SIZE + ptr_header->size);
        prev_header->next = ptr_header->next;
        if (ptr_header == last_alloc_ptr) {
            last_alloc_ptr = prev_header;
        }
        ptr_header = prev_header;
        if (ptr_header->next != NULL) {
            ((header*)ptr_header->next)->prev = ptr_header;
        }
    }

    //check if we can merge with next block
    header* next_header = (header*)(ptr_header->next);
    if (next_header && next_header->is_free) {
        if (next_header == last_alloc_ptr) {
            last_alloc_ptr = ptr_header;
        }
        ptr_header->size += (HEADER_SIZE + next_header->size);
        ptr_header->next = next_header->next;
        if (ptr_header->next != NULL) {
            ((header*)ptr_header->next)->prev = ptr_header;
        }
    }

    if (first_free_ptr == NULL || ((header*)first_free_ptr > ptr_header)) {
        first_free_ptr = ptr_header;
    }
    return true;
}








