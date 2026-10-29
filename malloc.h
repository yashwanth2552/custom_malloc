#include <stdbool.h>
typedef struct {
    bool  is_free;
    int   magic;
    int   size;
    void* next;
    void* prev;
} header;

void* custom_malloc(int size);
bool custom_free(void* ptr);
void heap_dump();
