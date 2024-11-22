/* 
  Copied from brain-hackers/cpython-wince PC/wince_compatibility.h
*/

#ifndef COMPATIBILITY_H
#define COMPATIBILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <winerror.h>

#ifndef va_list
#include <stdarg.h>
#endif

#include <fcntl.h>

/* errno emulation */

#define EPERM 1   /* Operation not permitted */
#define ENOFILE 2 /* No such file or directory */
#define ENOENT 2
#define ESRCH 3   /* No such process */
#define EINTR 4   /* Interrupted function call */
#define EIO 5     /* Input/output error */
#define ENXIO 6   /* No such device or address */
#define E2BIG 7   /* Arg list too long */
#define ENOEXEC 8 /* Exec format error */
#define EBADF 9   /* Bad file descriptor */
#define ECHILD 10 /* No child processes */
#define EAGAIN 11 /* Resource temporarily unavailable */
#define ENOMEM 12 /* Not enough space */
#define EACCES 13 /* Permission denied */
#define EFAULT 14 /* Bad address */
/* 15 - Unknown Error */
#define EBUSY 16   /* strerror reports "Resource device" */
#define EEXIST 17  /* File exists */
#define EXDEV 18   /* Improper link (cross-device link?) */
#define ENODEV 19  /* No such device */
#define ENOTDIR 20 /* Not a directory */
#define EISDIR 21  /* Is a directory */
#define EINVAL 22  /* Invalid argument */
#define ENFILE 23  /* Too many open files in system */
#define EMFILE 24  /* Too many open files */
#define ENOTTY 25  /* Inappropriate I/O control operation */
/* 26 - Unknown Error */
#define EFBIG 27  /* File too large */
#define ENOSPC 28 /* No space left on device */
#define ESPIPE 29 /* Invalid seek (seek on a pipe?) */
#define EROFS 30  /* Read-only file system */
#define EMLINK 31 /* Too many links */
#define EPIPE 32  /* Broken pipe */
#define EDOM 33   /* Domain error (math functions) */
#define ERANGE 34 /* Result too large (possibly too small) */
/* 35 - Unknown Error */
#define EDEADLOCK 36 /* Resource deadlock avoided (non-Cyg) */
#define EDEADLK 36
/* 37 - Unknown Error */
#define ENAMETOOLONG 38 /* Filename too long (91 in Cyg?) */
#define ENOLCK 39       /* No locks available (46 in Cyg?) */
#define ENOSYS 40       /* Function not implemented (88 in Cyg?) */
#define ENOTEMPTY 41    /* Directory not empty (90 in Cyg?) */
#define EILSEQ 42       /* Illegal byte sequence */

#define ECONVERT 43 /* Convert error (FIXME-WINCE: Icouldn't find the correct value */

#define EWOULDBLOCK EAGAIN /* Operation would block */

#define ECONNRESET 104 /* Connection reset by peer */
#define EISCONN 106    /* Transport endpoint is already connected */

/*
 * Because we need a per-thread errno, we define a function
 * pointer that we can call to return a pointer to the errno
 * for the current thread.  Then we define a macro for errno
 * that dereferences this function's result.
 *
 * This makes it syntactically just like the "real" errno.
 *
 * Using a function pointer allows us to use a very fast
 * function when there are no threads running and a slower
 * function when there are multiple threads running.
 */
#ifdef errno
#undef errno
#endif
void wince_errno_new_thread(int *errno_pointer);
void wince_errno_thread_exit(void);
extern int * (*wince_errno_pointer_function)(void);
#define errno (*(*wince_errno_pointer_function)())

#define _errno errno

extern int _sys_nerr;
extern const char *_sys_errlist[];

char * strerror(int errnum);

DWORD FormatMessageA(DWORD, const void *, DWORD, DWORD, char *, DWORD, va_list *);

/* locale emulation */

#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MONETARY 3
#define LC_NUMERIC 4
#define LC_TIME 5
#define LC_MIN LC_ALL
#define LC_MAX LC_TIME

char * setlocale(int, const char *);

/* define _wstat */

typedef short _dev_t;
typedef short _ino_t;
typedef unsigned short _mode_t;
typedef long _off_t;

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

#define _S_IFIFO 0x1000 /* FIFO */
#define _S_IFCHR 0x2000 /* Character */
#define _S_IFBLK 0x3000 /* Block */
#define _S_IFDIR 0x4000 /* Directory */
#define _S_IFREG 0x8000 /* Regular */

#define _S_IFMT 0xF000 /* File type mask */

#define _S_IEXEC 0x0040
#define _S_IWRITE 0x0080
#define _S_IREAD 0x0100

#define _S_IRWXU (_S_IREAD | _S_IWRITE | _S_IEXEC)
#define _S_IXUSR _S_IEXEC
#define _S_IWUSR _S_IWRITE
#define _S_IRUSR _S_IREAD

#define _S_ISDIR(m) (((m)&_S_IFMT) == _S_IFDIR)
#define _S_ISFIFO(m) (((m)&_S_IFMT) == _S_IFIFO)
#define _S_ISCHR(m) (((m)&_S_IFMT) == _S_IFCHR)
#define _S_ISBLK(m) (((m)&_S_IFMT) == _S_IFBLK)
#define _S_ISREG(m) (((m)&_S_IFMT) == _S_IFREG)

#define S_IFIFO _S_IFIFO
#define S_IFCHR _S_IFCHR
#define S_IFBLK _S_IFBLK
#define S_IFDIR _S_IFDIR
#define S_IFREG _S_IFREG
#define S_IFMT _S_IFMT
#define S_IEXEC _S_IEXEC
#define S_IWRITE _S_IWRITE
#define S_IREAD _S_IREAD
#define S_IRWXU _S_IRWXU
#define S_IXUSR _S_IXUSR
#define S_IWUSR _S_IWUSR
#define S_IRUSR _S_IRUSR

#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#define S_ISFIFO(m) (((m)&S_IFMT) == S_IFIFO)
#define S_ISCHR(m) (((m)&S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m)&S_IFMT) == S_IFBLK)
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)

#ifndef _STAT_DEFINED
#define _STAT_DEFINED
#define _stat wince__stat
#define stat wince__stat
struct _stat {
    _dev_t st_dev;   /* Equivalent to drive number 0=A 1=B ... */
    _ino_t st_ino;   /* Always zero ? */
    _mode_t st_mode; /* See above constants */
    short st_nlink;  /* Number of links. */
    short st_uid;    /* User: Maybe significant on NT ? */
    short st_gid;    /* Group: Ditto */
    _dev_t st_rdev;  /* Seems useless (not even filled in) */
    _off_t st_size;  /* File size in bytes */
    time_t st_atime; /* Accessed date (always 00:00 hrs local
                      * on FAT) */
    time_t st_mtime; /* Modified time */
    time_t st_ctime; /* Creation time */
};
#endif

int _wstat(const wchar_t *, struct _stat *);

DWORD GetCurrentDirectoryW(DWORD, wchar_t *);
BOOL SetCurrentDirectoryW(const WCHAR *);
DWORD GetFullPathNameW(const wchar_t *, DWORD, wchar_t *, wchar_t **);

#ifndef _TM_DEFINED
#define _TM_DEFINED
#define tm wince_tm

struct tm {
    int tm_sec;   /* Seconds: 0-59 */
    int tm_min;   /* Minutes: 0-59 */
    int tm_hour;  /* Hours since midnight: 0-23 */
    int tm_mday;  /* Day of the month: 1-31 */
    int tm_mon;   /* Months *since* january: 0-11 */
    int tm_year;  /* Years since 1900 */
    int tm_wday;  /* Days since Sunday (0-6) */
    int tm_yday;  /* Days since Jan. 1: 0-365 */
    int tm_isdst; /* +1 Daylight Savings Time, 0 No DST,
                   * -1 don't know */
};
#endif

typedef long clock_t;

clock_t clock(void);
char * wince_asctime(const struct tm *);

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

BOOL wince_DeleteFileW(const wchar_t *);
BOOL DeleteFileA(const char *);
BOOL wince_MoveFileW(const wchar_t *, const wchar_t *);
BOOL MoveFileA(const char *, const char *);
#define DeleteFileW wince_DeleteFileW
#define MoveFileW wince_MoveFileW

wchar_t ** CommandLineToArgvW(const wchar_t *, int *);

#ifdef __cplusplus
}
#endif

#endif