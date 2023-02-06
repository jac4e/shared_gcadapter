#include <shared_adapter.h>
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

int main() {
    printf("Init\n");
    int r;
    libusb_device_handle *handle;
    struct payload_region *adapter_payload;
    int payload_size = 0;

    // Get adapter payload ptr
    uint16_t unique_num;
    adapter_payload = (struct payload_region*)get_adapter(&unique_num);
    
    if (adapter_payload == NULL){
        // error
        printf("Couldn't get adapter ptr");
        return -1;
    }

    printf("Adapter #%d | Map addr %6.6X\n", unique_num, adapter_payload);

    while (1) {
        // printf("Payload size: %d\n", payload_size);
        for (int j=0; j < adapter_payload->payload_size; j++) {
            printf("%x ", adapter_payload->controller_payload[j]);
        }
        printf("\n");
        // std::cout << s_controller_payload->tp.count() * 10e-7 << "\n";
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}