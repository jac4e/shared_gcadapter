#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */ 
#include <chrono>
#include <thread>
#include <string>
#include <iostream>

const int adapter_payload_size = 37;

struct controller_payload_region
{
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point now;
	bool error;
    int payload_size;
	unsigned char controller_payload[adapter_payload_size];
};

int main() {
    printf("Init\n");
    int r;
    libusb_device_handle *handle;

    unsigned char previous_payload[adapter_payload_size];
    unsigned char current_payload[adapter_payload_size];
    int current_payload_size = 0;
    int previous_payload_size = 0;


    struct controller_payload_region *shared_controller_payload_region;
    // Shared memory init
    int shared_mem_fd = shm_open("/gcadapter", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shared_mem_fd== -1) {
        // error
        printf("Shared memory error");
        return -1;
    }

    if (ftruncate(shared_mem_fd, adapter_payload_size)== -1){
        // error
        printf("truncate memory error");
        return -1;
    }

    shared_controller_payload_region = (struct controller_payload_region*)mmap(NULL, adapter_payload_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem_fd, 0);
    if (shared_controller_payload_region == MAP_FAILED){
        // error
        printf("map memory error");
        return -1;
    }

    printf("Map addr is %6.6X\n", shared_controller_payload_region);

    printf("libusb Init\n");
    r = libusb_init(NULL);
    if (r) {
        printf("libusb init error: %s\n", libusb_error_name(r));
    }

    printf("Libusb open device\n");
    handle = libusb_open_device_with_vid_pid(NULL, 0x057e, 0x0337);

    printf("libusb claim interface\n");
    r = libusb_detach_kernel_driver(handle,0);
    r = libusb_claim_interface(handle, 0);
    if (r) {
        printf("libusb claim error: %s\n", libusb_error_name(r));
    }
    r = libusb_control_transfer(handle, 0x21, 11, 0x0001, 0, NULL, 0, 1000);
    if (r) {
        printf("libusb control error: %s\n", libusb_error_name(r));
    }
    int tmp = 0;
	unsigned char payload = 0x13;
	libusb_interrupt_transfer(handle, 2, &payload, sizeof(payload), &tmp, 32);

    while (1) {
		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
        r = libusb_interrupt_transfer(handle, 129, current_payload, adapter_payload_size, &current_payload_size, 32);
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        if (r) {
            // printf("libusb interrupt error: %s\n", libusb_error_name(r));
            memcpy(current_payload,previous_payload,adapter_payload_size);
            current_payload_size = previous_payload_size;
            // continue;
        } else {
            memcpy(previous_payload,current_payload,adapter_payload_size);
            previous_payload_size = current_payload_size;
        }
        shared_controller_payload_region->start = start;
        memcpy(shared_controller_payload_region->controller_payload,current_payload,adapter_payload_size);
        shared_controller_payload_region->error = r;
        shared_controller_payload_region->payload_size = current_payload_size;
        shared_controller_payload_region->start = now;
        
        // printf("\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    printf("libusb close\n");
    libusb_close(handle);
}