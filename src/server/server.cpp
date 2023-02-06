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
#include <atomic>

const int adapter_payload_size = 37;
std::atomic<bool> stop{false};

struct adapters *adapters;

struct adapter
{
    // int fd;
    uint16_t index;
    uint16_t unique_num;
    libusb_device_handle *handle;
    struct payload_region *payload;
};

int initialize_adapter(libusb_device *device, struct adapter *adapter) {
    int r;
    // uint8_t endpoint_address = 129;
    // Endpoint address for which you want to get the max packet size0

    r = libusb_open(device, &(adapter->handle));
    if (r < 0) {
        std::cerr << "Error opening device: " << libusb_error_name(r) << std::endl;
        return r;
    }

    // Get adapter shared memory
    adapter->payload = (payload_region *)get_adapter_by_unique_num(adapter->unique_num);

    r = libusb_detach_kernel_driver(adapter->handle,0);
    r = libusb_claim_interface(adapter->handle, 0);
    if (r) {
        std::cerr << "Error claiming device: " << libusb_error_name(r) << std::endl;
        return r;
    }
    r = libusb_control_transfer(adapter->handle, 0x21, 11, 0x0001, 0, NULL, 0, 1000);
    if (r) {
        std::cerr << "Error control device: " << libusb_error_name(r) << std::endl;
        return r;
    }
    int tmp = 0;
	unsigned char payload = 0x13;
	r = libusb_interrupt_transfer(adapter->handle, 2, &payload, sizeof(payload), &tmp, 32);
    if (r) {
        std::cerr << "Error on initial interrupt transfer: " << libusb_error_name(r) << std::endl;
        return r;
    }
    return 0;
}

void adapter_func(libusb_device *device, int index) {
    struct adapter adapter;

    uint8_t bus = libusb_get_bus_number(device);
    uint8_t dev_addr = libusb_get_device_address(device);
    adapter.unique_num = bus + (dev_addr << 8 );
    adapter.index = index;

    unsigned char previous_payload[adapter_payload_size];
    unsigned char current_payload[adapter_payload_size];
    int current_payload_size = 0;
    int previous_payload_size = 0;

    initialize_adapter(device, &adapter);

    while (!stop)
    {
        std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
        int r = libusb_interrupt_transfer(adapter.handle, 129, current_payload, adapter_payload_size, &current_payload_size, 32);
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
        adapter.payload->start = start;
        memcpy(adapter.payload->controller_payload,current_payload,adapter_payload_size);
        adapter.payload->error = r;
        adapter.payload->payload_size = current_payload_size;
        adapter.payload->start = now;
    }
    
}

int main() {

    // TODO: method to identify multiple adapters
    
    // Get list of lib_usb with pid and vid
    printf("Init\n");
    libusb_context *context = NULL;
    int result = libusb_init(&context);
    if (result < 0) {
        std::cerr << "Error initializing libusb: " << libusb_error_name(result) << std::endl;
        return 1;
    }

    libusb_device **list;
    ssize_t count = libusb_get_device_list(context, &list);
    if (count < 0) {
        std::cerr << "Error getting device list: " << libusb_error_name(count) << std::endl;
        libusb_exit(context);
        return 1;
    }


    uint16_t vendor_id = 0x057e;  // Replace with desired vendor ID
    uint16_t product_id = 0x0337;  // Replace with desired product ID
    int fd;
    adapters = (struct adapters *)get_shared_memory(&fd,"/gcadapters", sizeof(struct adapters));
    adapters->count = 0;
    for (ssize_t i = 0; i < count; ++i) {
        libusb_device *device = list[i];
        libusb_device_descriptor desc;
        result = libusb_get_device_descriptor(device, &desc);
        if (result < 0) {
            std::cerr << "Error getting device descriptor: " << libusb_error_name(result) << std::endl;
            continue;
        }

        if (desc.idVendor == vendor_id && desc.idProduct == product_id) {

            uint8_t bus = libusb_get_bus_number(device);
            uint8_t dev_addr = libusb_get_device_address(device);
            adapters->unique_num[adapters->count] = bus + (dev_addr << 8 );
            adapters->status[adapters->count] = 0b00000000;

            std::cout << "Found Gamecube Adapter #" << bus + (dev_addr << 8 ) << std::endl;
            // std::cout << ;
            std::thread adapter_thread(adapter_func, device, adapters->count);
            adapter_thread.detach();

            adapters->count++;
        }
    }

    while(1) {
        sleep(1);
    }
    libusb_free_device_list(list, 1);
    
    libusb_exit(context);

    return 0;
}