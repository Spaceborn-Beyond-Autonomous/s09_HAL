/* Minimal Cortex-M3 startup for STM32F100. Vector table, Reset_Handler that
 * copies .data from flash to RAM, zeroes .bss, then calls main(). */
.syntax unified
.cpu cortex-m3
.thumb

.section .isr_vector, "a"
.word _estack           /* initial stack pointer */
.word Reset_Handler + 1  /* +1 = thumb bit */
.word Default_Handler + 1 /* NMI */
.word Default_Handler + 1 /* HardFault */
.word Default_Handler + 1 /* MemManage */
.word Default_Handler + 1 /* BusFault */
.word Default_Handler + 1 /* UsageFault */
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word SysTick_Handler + 1 /* SysTick */

.section .text.Reset_Handler
.thumb_func
.global Reset_Handler
Reset_Handler:
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
copy_loop:
    cmp r1, r2
    bcs copy_done
    ldr r3, [r0], #4
    str r3, [r1], #4
    b copy_loop
copy_done:
    ldr r1, =_sbss
    ldr r2, =_ebss
    movs r3, #0
zero_loop:
    cmp r1, r2
    bcs zero_done
    str r3, [r1], #4
    b zero_loop
zero_done:
    bl main
hang:
    b hang

.section .text.Default_Handler
.thumb_func
.global Default_Handler
Default_Handler:
    b Default_Handler
