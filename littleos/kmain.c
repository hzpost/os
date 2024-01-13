/** @file */

#include "check.h"
#include "constants.h"
#include "framebuffer.h"
#include "interrupts.h"
#include "keyboard.h"
#include "kheap.h"
#include "module.h"
#include "paging.h"
#include "segments.h"
#include "serial.h"
#include "stdio.h"

char welcome[] = "Little OS";

void kmain() {
    fb_clear();
    printf("Hello %s\n", welcome);
    multiboot_check();
    kernel_check();
    serial_write(welcome, sizeof(welcome));
    init_kb();
    init_gdt();
    init_idt();
    init_paging();
    // page_fault_check();
    init_kheap();
    // kheap_check();
    run_apps();
}
