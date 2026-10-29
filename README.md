Built to get better understanding of malloc, and free.
1)Treated allocated/freed blocks of heap as nodes in double linked list.
2)Every node has a header that stores
   a)prev
   b)next
   c)size
   d)magic
   e)is_free
3)prev and next points to previous and next blocks in heap.
4)size stores the actual size of block(excluding the header size)
5)is_free helps to know if the block is currently free or not, used especially while searching for free block when user requests for memory.
6)magic number helps us to check if the pointer passed by user to custom_free is valid or not (if it's allocated by custom_alloc or not).
7)We maintain first_free_ptr which points to first free block in heap.
8)When user requests heap memory, we start traversing from first_free_ptr until we find a block whose size (excluding header size) is greater than or equal to user requested size.
9)When the found free block has more size than requested size, we need to think of splitting.
10)We define splitting threshold as HEADER_SIZE+1, i.e the splitted node should have size of atleast 1 Byte + HEADER.
11)If block_size-requested_size > SPLITTING_THRESHOLD, split the node to create a new empty block.
12)If splitting threshold is not met, we can't split, just leave it.
13)If empty block of atleast requested size is not found, we use sbrk(requested_size) system call, that moves the heap boundary by requestd_size.
14)When user calls custom_free(ptr), first check if ptr is valid by checking magic number.
15)We need to take care of coalescing i.e look if prev is empty, next is empty.

Disadvantages:
1)Every malloc calls traverses the entire list of blocks i.e O(N).
2)Freeing blocks creates holes in heap.
3)If ptr is not freed, it results in memory leak!!So developers need to manually call free for all allocations.
4)For every allocation, we need HEADER which adds to more space.
5)need to implement locking in case of multi threaded environment degrading performance.


Let's work on arena allocator!!
