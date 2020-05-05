#include <linux/unistd.h>
SECURITY_FILTER_BEGIN(syscall)
{__NR_clock_gettime},
{__NR_nanosleep},

{__NR_futex},
{__NR_get_robust_list},
{__NR_set_robust_list},

{__NR_poll},
{__NR_select},

{__NR_read},
{__NR_write},
{__NR_readv},
{__NR_writev},
{__NR_pread64},
{__NR_getrandom},

{__NR_recvfrom},
{__NR_sendto},
{__NR_recvmsg},
{__NR_sendmsg},
{__NR_sendmmsg},

{__NR_close},
{__NR_openat},
{__NR_fstat},
{__NR_lseek},
{__NR_ftruncate},
{__NR_fcntl},
{__NR_ioctl},

{__NR_open},
{__NR_stat},
{__NR_access},
{__NR_chmod},

{__NR_link},
{__NR_unlink},
{__NR_symlink},
{__NR_readlink},

{__NR_getcwd},
{__NR_mkdir},
{__NR_getdents},

{__NR_memfd_create},
{__NR_eventfd2},

{__NR_pipe},
{__NR_pipe2},
{__NR_socketpair},

{__NR_socket},
{__NR_getsockopt},
{__NR_setsockopt},
{__NR_getsockname},
{__NR_getpeername},

{__NR_bind},
{__NR_listen},
{__NR_accept},
{__NR_connect},

{__NR_brk},
{__NR_madvise},
{__NR_mprotect},
{__NR_mmap},
{__NR_munmap},

{__NR_getuid},
{__NR_getgid},
{__NR_geteuid},
{__NR_getegid},

{__NR_getpid},
{__NR_prctl},

{__NR_rt_sigaction},
{__NR_rt_sigprocmask},
{__NR_rt_sigreturn},
{__NR_tgkill},

{__NR_clone},
{__NR_fork},
{__NR_execve},

{__NR_sysinfo},
{__NR_uname},

{__NR_exit_group},
{__NR_exit},
SECURITY_FILTER_END(syscall)
