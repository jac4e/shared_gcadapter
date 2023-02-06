#pragma once
#include <chrono>

#define ADAPTER_PAYLOAD_SIZE 37
#define NAME_MAX_LENGTH 14
//gcadapter65535 <- max name

struct payload_region
{
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point now;
	bool error;
    int payload_size;
	unsigned char controller_payload[ADAPTER_PAYLOAD_SIZE];
};

struct adapters {
    uint16_t count;
    uint16_t unique_num[std::numeric_limits<std::uint16_t>::max()];
    uint8_t status[std::numeric_limits<std::uint16_t>::max()*2];
};

// status bitmap
#define STATUS_PORTS12_LOCK 0b00000001
#define STATUS_PORTS34_LOCK 0b00000010 


void *get_shared_memory(int *shared_mem_fd, const char *name, int size);
void *get_adapter_by_unique_num(int unique_num);
void *get_adapter(uint16_t *unique_num);