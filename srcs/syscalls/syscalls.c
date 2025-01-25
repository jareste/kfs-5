#include "syscalls.h"
#include "../utils/stdint.h"
#include "../display/display.h"

int sys_exit(int status)
{
    printf("Syscall: exit(%d)\n", status);
    return 0;
}

int sys_write(int fd, const char* buf, size_t count)
{
    printf("Syscall: write(%d, %s, %d)\n", fd, buf, count);
    return count;
}

int sys_read(int fd, char* buf, size_t count)
{
    printf("Syscall: read(%d, %p, %d)\n", fd, buf, count);
    return 0;
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

void syscall_handler(registers reg, uint32_t intr_no, uint32_t err_code, error_state stack)
{
    puts("Syscall Handler\n");

    uint32_t syscall_number = reg.eax;
    uint32_t arg1 = reg.ebx;
    uint32_t arg2 = reg.ecx;
    uint32_t arg3 = reg.edx;

    printf("Syscall number: %d\n", syscall_number);
    printf("Arguments: %d, %s, %d\n", arg1, (const char*)arg2, arg3);

    uint32_t ret_value = 0;

    switch (syscall_number) {
        case SYS_EXIT:
            ret_value = sys_exit(arg1);
            break;
        case SYS_WRITE:
            ret_value = sys_write(arg1, (const char*)arg2, arg3);
            break;
        case SYS_READ:
            ret_value = sys_read(arg1, (char*)arg2, arg3);
            break;
        case SYS_OPEN:
            ret_value = sys_open((const char*)arg1, arg2);
            break;
        case SYS_CLOSE:
            ret_value = sys_close(arg1);
            break;
        default:
            printf("Unknown syscall: %d\n", syscall_number);
            ret_value = -1;
            break;
    }

    reg.eax = 10;

}

void debug_syscall_enter()
{
    printf("Entering syscall handler\n");
}

void debug_syscall_exit()
{
    printf("Exiting syscall handler\n");
}
