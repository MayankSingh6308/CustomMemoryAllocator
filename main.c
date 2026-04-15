#include <stdio.h>
#include <stdlib.h>
typedef struct MemoryBlockNode {
    size_t size;   
    int is_free;   
    int customer_id;  
    struct MemoryBlockNode* prev;
    struct MemoryBlockNode* next;
} MemoryBlockNode;

void* memory_pool = NULL;
MemoryBlockNode* head = NULL;

void init_memory_pool(size_t size); //make one large empty box
void* allocate_memory(int customer_id, size_t size); 
void deallocate_memory(void* ptr);
void display_memory_map();
MemoryBlockNode* get_node_from_ptr(void* ptr);

void* get_data_ptr(MemoryBlockNode* node) {
    return (void*)((char*)node + sizeof(MemoryBlockNode));
}

void init_memory_pool(size_t size) {
    if (memory_pool != NULL) {
        printf("Memory pool is already initialized.\n");
        return;
    }
    
    if (size <= sizeof(MemoryBlockNode)) {
        printf("Error: Pool size is too small.\n");
        return;
    }

    memory_pool = malloc(size);
    if (memory_pool == NULL) {
        printf("Error: Failed to allocate memory pool.\n");
        return;
    }

    head = (MemoryBlockNode*)memory_pool;
    head->size = size - sizeof(MemoryBlockNode);
    head->is_free = 1;
    head->customer_id = -1;
    head->prev = NULL;
    head->next = NULL;
}

void* allocate_memory(int customer_id, size_t size) {
    if (head == NULL) {
        printf("Error: Memory pool not initialized.\n");
        return NULL;
    }

    MemoryBlockNode* current = head;
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            if (current->size > size + sizeof(MemoryBlockNode)) {
                MemoryBlockNode* new_node = (MemoryBlockNode*)((char*)get_data_ptr(current) + size);
                new_node->size = current->size - size - sizeof(MemoryBlockNode);
                new_node->is_free = 1;
                new_node->customer_id = -1;
                new_node->prev = current;
                new_node->next = current->next;
                
                if (current->next != NULL) {
                    current->next->prev = new_node;
                }
                current->next = new_node;
                
                current->size = size;
            }
            
            current->is_free = 0;
            current->customer_id = customer_id;
            return get_data_ptr(current);
        }
        current = current->next;
    }

    return NULL;
}

void deallocate_memory(void* ptr) {
    if (ptr == NULL || head == NULL) {
        printf("Invalid pointer or memory pool not initialized.\n");
        return;
    }

    MemoryBlockNode* node_to_free = get_node_from_ptr(ptr);
    if (node_to_free == NULL || node_to_free->is_free) {
        printf("Error: Invalid pointer or block already free.\n");
        return;
    }

    node_to_free->is_free = 1;
    node_to_free->customer_id = -1;

    if (node_to_free->next != NULL && node_to_free->next->is_free) {
        node_to_free->size += node_to_free->next->size + sizeof(MemoryBlockNode);
        
        MemoryBlockNode* temp = node_to_free->next;
        node_to_free->next = temp->next;
        if (temp->next != NULL) {
            temp->next->prev = node_to_free;
        }
    }
    
    if (node_to_free->prev != NULL && node_to_free->prev->is_free) {
        MemoryBlockNode* prev_node = node_to_free->prev;
        prev_node->size += node_to_free->size + sizeof(MemoryBlockNode);
        prev_node->next = node_to_free->next;
        if (node_to_free->next != NULL) {
            node_to_free->next->prev = prev_node;
        }
    }
}

void display_memory_map() {
    if (head == NULL) {
        printf("Memory pool is empty or not initialized.\n");
        return;
    }

    MemoryBlockNode* current = head;
    printf("Memory Map:\n");
    int block_number = 0;
    while (current != NULL) {
        printf("Block %d: ", ++block_number);
        if (current->is_free) {
            printf("[FREE] Size: %zu bytes\n", current->size);
        } else {
            printf("[ALLOCATED] Customer ID: %d, Size: %zu bytes\n", current->customer_id, current->size);
        }
        current = current->next;
    }
}

MemoryBlockNode* get_node_from_ptr(void* ptr) {
    return (MemoryBlockNode*)((char*)ptr - sizeof(MemoryBlockNode));
}

int main() {
    int choice;
    size_t pool_size;
    int customer_id;
    size_t allocation_size;
    void** allocated_pointers = NULL;
    int allocation_count = 0;

    printf("Welcome to the Customer Memory Allocator!\n");
    printf("--------------------------------------\n");

    printf("Enter the total size of the memory pool (in bytes): ");
    scanf("%zu", &pool_size);
    init_memory_pool(pool_size);
    if(memory_pool == NULL) return 1;
    printf("Memory pool of %zu bytes initialized successfully.\n", pool_size);

    do {
        printf("\n--- Main Menu ---\n");
        printf("1. Allocate Memory for a Customer\n");
        printf("2. Deallocate Memory\n");
        printf("3. Display Memory Map\n");
        printf("4. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("\nEnter customer ID: ");
                scanf("%d", &customer_id);
                printf("Enter memory size to allocate: ");
                scanf("%zu", &allocation_size);
                
                void* new_ptr = allocate_memory(customer_id, allocation_size);
                if (new_ptr != NULL) {
                    printf("Memory of %zu bytes allocated successfully for customer %d.\n", allocation_size, customer_id);
                    allocation_count++;
                    allocated_pointers = realloc(allocated_pointers, allocation_count * sizeof(void*));
                    allocated_pointers[allocation_count - 1] = new_ptr;
                } else {
                    printf("Error: Could not allocate memory of %zu bytes.\n", allocation_size);
                }
                break;
            case 2:
                if (allocation_count > 0) {
                    int ptr_index;
                    printf("\nCurrently allocated blocks:\n");
                    for (int i = 0; i < allocation_count; i++) {
                         MemoryBlockNode* node = get_node_from_ptr(allocated_pointers[i]);
                         printf("%d: Customer %d, Size %zu\n", i + 1, node->customer_id, node->size);
                    }
                    printf("Enter the number of the block to deallocate: ");
                    scanf("%d", &ptr_index);
                    if (ptr_index > 0 && ptr_index <= allocation_count) {
                        deallocate_memory(allocated_pointers[ptr_index - 1]);
                        printf("\nMemory deallocated successfully.\n");
                        for(int i = ptr_index - 1; i < allocation_count - 1; i++) {
                           allocated_pointers[i] = allocated_pointers[i+1];
                        }
                        allocation_count--;
                        if (allocation_count == 0) {
                            free(allocated_pointers);
                            allocated_pointers = NULL;
                        } else {
                            allocated_pointers = realloc(allocated_pointers, allocation_count * sizeof(void*));
                        }
                    } else {
                         printf("Invalid selection.\n");
                    }
                } else {
                    printf("\nError: No memory has been allocated to deallocate.\n");
                }
                break;
            case 3:
                printf("\n--- Current Memory Map ---\n");
                display_memory_map();
                break;
            case 4:
                printf("\nExiting program. Goodbye! \n");
                if (memory_pool != NULL) {
                    free(memory_pool);
                }
                if (allocated_pointers != NULL) {
                    free(allocated_pointers);
                }
                break;
            default:
                printf("\nInvalid choice. Please try again.\n");
        }
    } while (choice != 4);

    return 0;
}
