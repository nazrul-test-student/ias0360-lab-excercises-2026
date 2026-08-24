#include "pico/stdlib.h"
#include "context.h"
#include "hardware/watchdog.h"
#include "pico/platform.h"            // __disable_irq()
#include "hardware/regs/addressmap.h"
#include "hardware/regs/m0plus.h"
#include "hardware/structs/scb.h"

typedef void (*entry_t)(void);
__attribute__((noreturn, noinline))
void jump_to_image(uint32_t offset)
{
    uint32_t app_base = XIP_BASE + offset;
    printf("Jumping to image at address 0x%X\n", app_base);
    __asm volatile ("cpsid i" ::: "memory");
    // Disable SysTick (no CMSIS needed)
    *(volatile uint32_t *)(PPB_BASE + M0PLUS_SYST_CSR_OFFSET) = 0;
    *(volatile uint32_t *)(PPB_BASE + M0PLUS_SYST_CVR_OFFSET) = 0;
    // Clear & disable NVIC interrupts
    for (int i = 0; i < 8; ++i) {
        *(volatile uint32_t *)(PPB_BASE + M0PLUS_NVIC_ICER_OFFSET + i*4) = 0xFFFFFFFFu;
        *(volatile uint32_t *)(PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET + i*4) = 0xFFFFFFFFu;
    }
    const uint32_t *vt = (const uint32_t *)app_base;
    uint32_t msp   = vt[0];
    uint32_t reset = vt[1];
    __asm volatile("dsb 0xF; isb 0xF" ::: "memory");
    scb_hw->vtor = app_base;                // VTOR without CMSIS
    __asm volatile("dsb 0xF; isb 0xF" ::: "memory");
    __asm volatile("msr msp, %0" :: "r"(msp) : "memory");
    __asm volatile ("cpsie i" ::: "memory");
    ((entry_t)reset)();
    __builtin_unreachable();
}