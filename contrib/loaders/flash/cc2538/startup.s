        .section .entry
        .global entry
        .syntax unified
        .thumb_func
entry:
        /* Disable interrupts */
        cpsid i

        /* Privileged mode, main stack, no floating point */
        mov r0, #0
        msr control, r0
        isb

        /* Load initial stack pointer */
        ldr r0, =_estack
        mov sp, r0
        isb

        /* Clear BSS */
        mov r0, #0
        ldr r1, =_bss
        ldr r2, =_ebss
1:
        cmp r1, r2
        it lt
        strlt r0, [r1], #4
        blt 1b

        /* No need to copy initialized data, since it's already in RAM */

        /* It's safe to call C code now */
        bl __libc_init_array
        bl main

        /* Main returned -- hang */
        b .

