#include "syscalls.h"
#include "../utils/stdint.h"
#include "../display/display.h"
#include "../tasks/task.h"
#include "../keyboard/signals.h"

typedef int (*syscall_handler_6_t)(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6);
typedef int (*syscall_handler_5_t)(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
typedef int (*syscall_handler_4_t)(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);
typedef int (*syscall_handler_3_t)(uint32_t arg1, uint32_t arg2, uint32_t arg3);
typedef int (*syscall_handler_2_t)(uint32_t arg1, uint32_t arg2);
typedef int (*syscall_handler_1_t)(uint32_t arg1);
typedef int (*syscall_handler_0_t)();

typedef enum
{
    RET_INT = 0,
    RET_PTR = 1,
    RET_SIZE = 2,
} ret_value_size;

typedef union
{
    int int_value;
    void* ptr_value;
    size_t size_value;
} syscall_return_t;

typedef union
{
    syscall_handler_6_t handler_6;
    syscall_handler_5_t handler_5;
    syscall_handler_4_t handler_4;
    syscall_handler_3_t handler_3;
    syscall_handler_2_t handler_2;
    syscall_handler_1_t handler_1;
    syscall_handler_0_t handler_0;
} syscall_handler_t;

typedef struct
{
    syscall_return_t ret_value;
    syscall_handler_t handler;
    uint8_t num_args;
    ret_value_size ret_value_entry;
} syscall_entry_t;

int sys_exit(int status)
{
    printf("Syscall: exit(%d)\n", status);
    return 0;
}

int sys_write(int fd, const char* buf, size_t count)
{
    if (!buf || count == 0)
    {
        printf("Write: Invalid buffer or count\n");
        return -1;
    }
    puts(buf);
    return count;
}

int sys_read(int fd, char* buf, size_t count)
{
    if (!buf || count == 0)
    {
        printf("Read: Invalid buffer or count\n");
        return -1;
    }

    const char* data = "123456789";
    size_t data_len = strlen(data);

    if (count > data_len)
    {
        count = data_len;
    }

    memcpy(buf, data, count);

    // printf("Syscall: read(%d, %p, %d) - Filled buffer with: '", fd, buf, count);
    // for (size_t i = 0; i < count; i++)
    // {
    //     putc(buf[i]);
    // }
    // puts("'\n");

    return count;
}

int sys_open(const char* path, int flags)
{
    printf("Syscall: open(%s, %d)\n", path, flags);
    return 0;
}

int sys_close(int fd)
{
    printf("Syscall: close(%d)\n", fd);
    return 0;
}

int sys_get_pid()
{
    printf("Syscall: getpid()\n");
    return 0;
}

void sys_sleep(uint32_t seconds)
{
    printf("Syscall: sleep(%d)\n", seconds);
}

int sys_kill(uint32_t pid, uint32_t signal)
{
    return _kill(pid, signal);
}

int sys_signal(int signal, signal_handler_t handler)
{
    return _signal(signal, handler);
}

syscall_entry_t syscall_table[MAX_SYSCALLS] = {
    { .handler.handler_1 = (syscall_handler_1_t)sys_exit,       .num_args = 1, .ret_value_entry = RET_INT },
    { .handler.handler_3 = (syscall_handler_3_t)sys_write,      .num_args = 3, .ret_value_entry = RET_SIZE },
    { .handler.handler_3 = (syscall_handler_3_t)sys_read,       .num_args = 3, .ret_value_entry = RET_SIZE },
    { .handler.handler_2 = (syscall_handler_2_t)sys_open,       .num_args = 2, .ret_value_entry = RET_INT },
    { .handler.handler_1 = (syscall_handler_1_t)sys_close,      .num_args = 1, .ret_value_entry = RET_INT },
    { .handler.handler_0 = (syscall_handler_0_t)sys_get_pid,    .num_args = 0, .ret_value_entry = RET_INT },
    { .handler.handler_1 = (syscall_handler_1_t)sys_sleep,      .num_args = 1, .ret_value_entry = RET_INT },
    { .handler.handler_2 = (syscall_handler_2_t)sys_kill,       .num_args = 2, .ret_value_entry = RET_INT },
    { .handler.handler_2 = (syscall_handler_2_t)sys_signal,     .num_args = 2, .ret_value_entry = RET_INT },
};

int syscall_handler(registers reg, uint32_t intr_no, uint32_t err_code, error_state stack)
{
    uint32_t syscall_number = reg.eax;
    uint32_t arg1 = reg.ebx;
    uint32_t arg2 = reg.ecx;
    uint32_t arg3 = reg.edx;
    uint32_t arg4 = reg.esi;
    uint32_t arg5 = reg.edi;
    uint32_t arg6 = reg.ebp;

    if (syscall_number >= MAX_SYSCALLS)
    {
        printf("Unknown syscall: %d\n", syscall_number);
        return -1;
    }
    scheduler();

    syscall_entry_t entry = syscall_table[syscall_number];

    // printf("Syscall: %d\n", syscall_number);
    syscall_return_t ret_value;
    switch (entry.num_args)
    {
        case 0:
            ret_value.int_value = entry.handler.handler_0();
            break;
        case 1:
            ret_value.int_value = entry.handler.handler_1(arg1);
            break;
        case 2:
            ret_value.int_value = entry.handler.handler_2(arg1, arg2);
            break;
        case 3:
            ret_value.int_value = entry.handler.handler_3(arg1, arg2, arg3);
            break;
        case 4:
            ret_value.int_value = entry.handler.handler_4(arg1, arg2, arg3, arg4);
            break;
        case 5:
            ret_value.int_value = entry.handler.handler_5(arg1, arg2, arg3, arg4, arg5);
            break;
        case 6:
            ret_value.int_value = entry.handler.handler_6(arg1, arg2, arg3, arg4, arg5, arg6);
            break;
        default:
            printf("Invalid number of arguments for syscall %d\n", syscall_number);
            ret_value.int_value = -1;
            break;
    }

    scheduler();

    return ret_value.int_value;
}
