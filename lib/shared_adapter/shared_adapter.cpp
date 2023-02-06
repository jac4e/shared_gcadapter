#include "shared_adapter.h"
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include <cstdio>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */
#include <string>
#include <iostream>

void *get_shared_memory(int *shared_mem_fd, const char *name, int size){
    // Shared memory init
    *shared_mem_fd = shm_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (*shared_mem_fd == -1) {
        // error
        printf("Shared memory error");
        return NULL;
    }

    if (ftruncate(*shared_mem_fd, size)== -1){
        // error
        printf("truncate memory error");
        return NULL;
    }

    void* shared_region = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, *shared_mem_fd, 0);
    if (shared_region == MAP_FAILED){
        // error
        printf("map memory error");
        return NULL;
    }

    return shared_region;
}

void *get_adapter_by_unique_num(int unique_num) {
    // make adapter name
    char name [NAME_MAX_LENGTH+1] = {};
    snprintf(name, NAME_MAX_LENGTH, "/gcadapter%s", std::to_string(unique_num).c_str());
    int fd;
    return get_shared_memory(&fd, name, ADAPTER_PAYLOAD_SIZE);
}

void *get_adapter(uint16_t *unique_num) {
    // Get adapter list
    int fd;
    struct adapters *adapters = (struct adapters *)get_shared_memory(&fd,"/gcadapters", sizeof(struct adapters));

    (*unique_num) = 0;

    printf("count: %d\n", adapters->count);
    for (size_t i = 0; i < adapters->count; i++)
    {
        printf("num: %d, status: %d", adapters->unique_num[i], adapters->status[0]);
        printf("\t| %d %d\n", (adapters->status[i] & STATUS_PORTS12_LOCK), (adapters->status[i] & STATUS_PORTS34_LOCK));
        // Check if adapter is taken
        if ((adapters->status[i] & STATUS_PORTS12_LOCK) && (adapters->status[i] & STATUS_PORTS34_LOCK)) {
            // Both sides of adapter taken
            continue;
        }
        if ((adapters->status[i] & STATUS_PORTS12_LOCK)) {
            // Only Ports 1 and 2 are taken
            adapters->status[i]|=STATUS_PORTS34_LOCK;
            (*unique_num) = adapters->unique_num[i];
        } else if ((adapters->status[i] & STATUS_PORTS34_LOCK)) {
            // Only Ports 1 and 2 are taken
            adapters->status[i]|=STATUS_PORTS12_LOCK;
            (*unique_num) = adapters->unique_num[i];
        } else {
            // no ports taken
            adapters->status[i]|=STATUS_PORTS12_LOCK;
            (*unique_num) = adapters->unique_num[i];
        }
    }

    if ((*unique_num) == 0) {
        std::cerr << "Could not find unused adapter side" << std::endl;
        return NULL;
    }

    return get_adapter_by_unique_num(*unique_num);
}