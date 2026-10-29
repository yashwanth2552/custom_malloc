#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "malloc.h"

// Forward declarations of your allocator functions so the test can see them
void* custom_malloc(int act_req_size);
bool custom_free(void* ptr);
void heap_dump();

extern void* first_free_ptr;
extern void* last_alloc_ptr;
extern void* heap_start_ptr;

void run_deadly_tests() {
    printf("\n=== RUNNING CRITICAL ADVERSARIAL ALLOCATOR TEST SUITE ===\n");

    // ----------------------------------------------------
    // TEST 1: The Alignment Chaos & Odd Sizing
    // Stress tests the 8/16-byte rounding math rules
    // ----------------------------------------------------
    printf("\n[Test 1] Alignment Chaos...\n");
    void* p1 = custom_malloc(1);   // 1 byte request -> should round up to 8 bytes
    void* p2 = custom_malloc(7);   // 7 byte request -> should round up to 8 bytes
    void* p3 = custom_malloc(13);  // 13 byte request -> should round up to 16 bytes
    
    // Check if the data addresses returned are perfectly 8-byte aligned
    if (((uintptr_t)p1 % 8 != 0) || ((uintptr_t)p2 % 8 != 0) || ((uintptr_t)p3 % 8 != 0)) {
        printf("❌ FAILED TEST 1: Memory address returned is NOT aligned to an 8-byte boundary!\n");
    } else {
        printf("✅ Passed Test 1: Alignment confirmed.\n");
    }
    heap_dump();

    // ----------------------------------------------------
    // TEST 2: The Splitting Edge-Case / Ghost Fragment
    // Asks for space that leaves exactly 1 byte left *before* considering the header.
    // Tests if the threshold condition prevents creating a ghost/illegal negative header block.
    // ----------------------------------------------------
    printf("\n[Test 2] Freeing p2 to create a slot, then testing Splitting Threshold...\n");
    custom_free(p2); // Free up p2 slot (which is 8 bytes of user data space)
    
    // If our threshold requires space > HEADER_SIZE + 1, this should NOT split, 
    // it should just consume the whole block without throwing errors.
    void* p2_retry = custom_malloc(7); 
    printf("✅ Finished Test 2. Checking layout to ensure no microscopic broken fragments exist:\n");
    heap_dump();

    // Clean up all to reset heap state cleanly for test 3
    custom_free(p1);
    custom_free(p2_retry);
    custom_free(p3);

    // ----------------------------------------------------
    // TEST 3: The Multi-Block Sandbox (Coalescing Gauntlet)
    // Allocates 5 continuous blocks, then frees them in an order
    // designed to break pointer tracking and test forward/backward merging.
    // ----------------------------------------------------
    printf("\n[Test 3] Setting up Coalescing Gauntlet...\n");
    void* a = custom_malloc(16);
    void* b = custom_malloc(16);
    void* c = custom_malloc(16);
    void* d = custom_malloc(16);
    void* e = custom_malloc(16);
    
    printf("--- Baseline state before complex frees ---\n");
    heap_dump();

    // Free the middle block first (No immediate merges available)
    printf("\nFreeing middle block [C]...\n");
    custom_free(c);
    
    // Free the trailing block [E] (Tests your last_alloc_ptr updates)
    printf("\nFreeing end block [E] (Tests last_alloc_ptr boundaries)...\n");
    custom_free(e);
    
    // Free block [D] -> This triggers a double-coalesce!
    // It must merge with C (before it) AND E (after it) simultaneously.
    // This will thoroughly test your pointer rewires and last_alloc_ptr changes.
    printf("\nFreeing block [D] -> Triggers a massive forward + backward dual merge!\n");
    custom_free(d);
    heap_dump();

    // Clean up remaining blocks to test full heap collapse back to a single free block
    printf("\nFreeing remaining blocks [A] and [B] to trigger complete physical consolidation...\n");
    custom_free(a);
    custom_free(b);
    heap_dump();

    // ----------------------------------------------------
    // TEST 4: The Poison Pill (Canary Validation)
    // Intentionally feeds an invalid address to custom_free to verify defense strategy.
    // ----------------------------------------------------
    printf("\n[Test 4] Poison Pill Check (Invalid Free Attempt)...\n");
    uint64_t fake_object[10] = {0}; 
    void* fake_ptr = &fake_object[5]; // Points directly to stack space, not our heap!

    printf("Attempting to free a fake pointer... (Should print error and return false cleanly)\n");
    bool free_status = custom_free(fake_ptr);
    
    if (free_status == false) {
        printf("✅ Passed Test 4: Allocator rejected the poison pill successfully without throwing a segmentation fault!\n");
    } else {
        printf("❌ FAILED TEST 4: Allocator blindly accepted a fake pointer! Memory corruption imminent.\n");
    }
}

int main() {
    run_deadly_tests();
    printf("\n=== TEST RUN EXECUTED ===\n");
    return 0;
}

