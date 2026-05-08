.section .data
.align 4
.globl simulated_cpu_state
.type simulated_cpu_state, @object
.size simulated_cpu_state, 251520 # 251520 = 4192 * 60
simulated_cpu_state:
    .space 251520 # 251520 = 4192 * 60