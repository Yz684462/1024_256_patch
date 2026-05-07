.section .data
.align 4
.globl big_data
.type big_data, @object
.size big_data, 4192
big_data:
    .space 4192
    # 或者需要初始值：
    # .byte 0x00, 0x01, 0x02
    # .fill 4189, 1, 0