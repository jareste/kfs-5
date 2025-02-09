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

bool syscall_happening = false;

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
    // syscall_handler_6_t handler_6;
    // syscall_handler_5_t handler_5;
    // syscall_handler_4_t handler_4;
    // syscall_handler_3_t handler_3;
    // syscall_handler_2_t handler_2;
    // syscall_handler_1_t handler_1;
    // syscall_handler_0_t handler_0;
    void* handler;
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
    return 1;
}

int sys_write(int fd, const char* buf, size_t count)
{
    if (!buf || count == 0)
    {
        printf("Write: Invalid buffer or count\n");
        return -1;
    }
    if (fd < 0 || fd > 2)
    {
        // printf("Write: Invalid file descriptor\n");
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
    return 1;
}

int sys_close(int fd)
{
    printf("Syscall: close(%d)\n", fd);
    return 1;
}

int sys_get_pid()
{
    return get_current_task()->pid;
}

void sys_sleep(uint32_t seconds)
{
    printf("Syscall: sleep(%d)\n", seconds);
}

int sys_kill(uint32_t pid, uint32_t signal)
{
    return _kill(pid, signal);
}

int sys_signal(uint32_t pid, signal_handler_t hand)
{
    return _signal(pid, hand);
}

pid_t fork()
{
    return _fork();
}

syscall_entry_t syscall_table[SYS_MAX_SYSCALL];

int syscall_handler(registers reg, uint32_t intr_no, uint32_t err_code, error_state stack)
{
    uint32_t syscall_number = reg.eax;
    uint32_t arg1 = reg.ebx;
    uint32_t arg2 = reg.ecx;
    uint32_t arg3 = reg.edx;
    uint32_t arg4 = reg.esi;
    uint32_t arg5 = reg.edi;
    uint32_t arg6 = reg.ebp;

    // printf("Syscall: %d\n", syscall_number);
    if (syscall_number >= SYS_MAX_SYSCALL || syscall_table[syscall_number].handler.handler == NULL)
    {
        printf("Unknown syscall: %d\n", syscall_number);
        return -1;
    }
    // printf("syscall happening: %d\n", syscall_happening);
    while (syscall_happening)
    {
        // puts("Syscall happening\n");
        scheduler();
    }
    syscall_happening = true;

    syscall_entry_t entry = syscall_table[syscall_number];

    // printf("Syscall: %d\n", syscall_number);
    syscall_return_t ret_value;
    switch (entry.num_args)
    {
        case 0:
            ret_value.int_value = ((syscall_handler_0_t)entry.handler.handler)();
            break;
        case 1:
            ret_value.int_value = ((syscall_handler_1_t)entry.handler.handler)(arg1);
            break;
        case 2:
            ret_value.int_value = ((syscall_handler_2_t)entry.handler.handler)(reg.ebx, reg.ecx);
            break;
        case 3:
            ret_value.int_value = ((syscall_handler_3_t)entry.handler.handler)(arg1, arg2, arg3);
            break;
        case 4:
            ret_value.int_value = ((syscall_handler_4_t)entry.handler.handler)(arg1, arg2, arg3, arg4);
            break;
        case 5:
            ret_value.int_value = ((syscall_handler_5_t)entry.handler.handler)(arg1, arg2, arg3, arg4, arg5);
            break;
        case 6:
            ret_value.int_value = ((syscall_handler_6_t)entry.handler.handler)(arg1, arg2, arg3, arg4, arg5, arg6);
            break;
        default:
            printf("Invalid number of arguments for syscall %d\n", syscall_number);
            ret_value.int_value = -1;
            break;
    }

    // scheduler();

    syscall_happening = false;
    return ret_value.int_value;
}

void init_syscalls()
{
    memset(syscall_table, 0, sizeof(syscall_table));

    syscall_table[SYS_EXIT] = (syscall_entry_t){
        .ret_value_entry = RET_INT,
        .num_args = 1,
        .handler.handler = (void*)sys_exit,
    };

    syscall_table[SYS_WRITE] = (syscall_entry_t){
        .ret_value_entry = RET_SIZE,
        .num_args = 3,
        .handler.handler = (void*)sys_write,
    };

    syscall_table[SYS_READ] = (syscall_entry_t){
        .ret_value_entry = RET_SIZE,
        .num_args = 3,
        .handler.handler = (void*)sys_read,
    };

    syscall_table[SYS_OPEN] = (syscall_entry_t){
        .ret_value_entry = RET_INT,
        .num_args = 2,
        .handler.handler = (void*)sys_open,
    };

    syscall_table[SYS_CLOSE] = (syscall_entry_t){
        .ret_value_entry = RET_INT,
        .num_args = 1,
        .handler.handler = (void*)sys_close,
    };

    syscall_table[SYS_GETPID] = (syscall_entry_t){
        .ret_value_entry = RET_INT,
        .num_args = 0,
        .handler.handler = (void*)sys_get_pid,
    };

    // syscall_table[SYS_SLEEP] = (syscall_entry_t){
    //     .ret_value_entry = RET_INT,
    //     .num_args = 1,
    //     .handler.handler_1 = sys_sleep,
    // };

    syscall_table[SYS_SIGNAL] = (syscall_entry_t){
        .ret_value_entry = RET_INT,
        .num_args = 2,
        .handler.handler = (void*)sys_signal,
    };

    syscall_table[SYS_KILL] = (syscall_entry_t){
        .ret_value_entry = RET_INT,
        .num_args = 2,
        .handler.handler = (void*)sys_kill,
    };
}
