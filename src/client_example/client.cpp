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

    struct controller_payload_region *s_controller_payload;
    int payload_size = 0;


    // Shared memory init
    int fd;
    fd = shm_open("/gcadapter", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd== -1) {
        // error
        printf("Shared memory error");
        return -1;
    }

    if (ftruncate(fd, adapter_payload_size)== -1){
        // error
        printf("truncate memory error");
        return -1;
    }

    s_controller_payload = (struct controller_payload_region*)mmap(NULL, adapter_payload_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (s_controller_payload == MAP_FAILED){
        // error
        printf("map memory error");
        return -1;
    }

    printf("Map addr is %6.6X\n", s_controller_payload);

    while (1) {
        // printf("Payload size: %d\n", payload_size);
        for (int j=0; j < adapter_payload_size; j++) {
            printf("%x ", s_controller_payload->controller_payload[j]);
        }
        printf("\n");
        // std::cout << s_controller_payload->tp.count() * 10e-7 << "\n";
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}