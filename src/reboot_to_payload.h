#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <switch.h>

void do_iram_dram_copy(void* buf, uintptr_t iram_addr, size_t size, int option);
void copy_to_iram(uintptr_t iram_addr, void* buf, size_t size);
void copy_from_iram(void* buf, uintptr_t iram_addr, size_t size);
void clear_iram(void);
void rtp_iram(void);
int reboot_to_payload();