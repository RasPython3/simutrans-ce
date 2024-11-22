/* 
  Copied from brain-hackers/cpython-wince PC/wince_compatibility.c
*/

#include "compatibility.h"

#include <limits.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/********************************************************/
/*							*/
/* Errno emulation:					*/
/*	There is no errno on Windows CE and we need	*/
/*	to make it per-thread.  So we have a function	*/
/*	that returns a pointer to the errno for the	*/
/*	current thread.					*/
/*							*/
/*	If there is ONLY the main thread then we can	*/
/* 	quickly return some static storage.		*/
/*							*/
/*	If we have multiple threads running, we use	*/
/*	Thread-Local Storage to hold the pointer	*/
/*							*/
/********************************************************/

/* _sys_nerr and _sys_errlist */
static char undefined_error[] = "Undefined error";

const char *_sys_errlist[] = {
    undefined_error,                       /*  0 - ENOERROR */
    undefined_error,                       /*  1 - EPERM */
    "No such file or directory",           /*  2 - ENOENT */
    "No such process",                     /*  3 - ESRCH */
    "Interrupted system call",             /*  4 - EINTR */
    undefined_error,                       /*  5 - EIO */
    "Device not configured",               /*  6 - ENXIO */
    undefined_error,                       /*  7 - E2BIG */
    undefined_error,                       /*  8 - ENOEXEC */
    undefined_error,                       /*  9 - EBADF */
    undefined_error,                       /* 10 - ECHILD */
    undefined_error,                       /* 11 - EDEADLK */
    undefined_error,                       /* 12 - ENOMEM */
    "Permission denied",                   /* 13 - EACCES */
    "Bad access",                          /* 14 - EFAULT */
    undefined_error,                       /* 15 - ENOTBLK */
    "File is busy",                        /* 16 - EBUSY */
    "File exists",                         /* 17 - EEXIST */
    undefined_error,                       /* 18 - EXDEV */
    undefined_error,                       /* 19 - ENODEV */
    "Is not a directory",                  /* 20 - ENOTDIR */
    "Is a directory",                      /* 21 - EISDIR */
    "Invalid argument",                    /* 22 - EINVAL */
    "Too many open files in system",       /* 23 - ENFILE */
    "Too many open files",                 /* 24 - EMFILE */
    "Inappropriate I/O control operation", /* 25 - NOTTY */
    undefined_error,                       /* 26 - unknown */
    "File too large",                      /* 27 - EFBIG */
    "No space left on device",             /* 28 - ENOSPC */
    "Invalid seek",                        /* 29 - ESPIPE */
    "Read-only file systems",              /* 30 - EROFS */
    "Too many links",                      /* 31 - EMLINK */
    "Broken pipe",                         /* 32 - EPIPE */
    "Domain error",                        /* 33 - EDOM */
    "Result too large",                    /* 34 - ERANGE */
    undefined_error,                       /* 35 - unknown */
    "Resource deadlock avoided",           /* 36 - EDEADLK */
    undefined_error,                       /* 37 - unknown */
    "File name too long",                  /* 38 - ENAMETOOLONG */
    "No locks available",                  /* 39 - ENOLCK */
    "Functions not implemented",           /* 40 - ENOSYS */
    "Directory not empty",                 /* 41 - ENOTEMPTY */
    "Illegal byte sequence",               /* 42 - EILSEQ */
    "Convert failed"                       /* 43 - ECONVERT */
};

/* This is set to zero for now because it is only used in Python/errors.c,
   and setting it to zero will cause Python to call FormatMessage instead */
int _sys_nerr = 0; /*sizeof(_sys_errlist)/sizeof(_sys_errlist[0]);*/

/* Function pointer for returning errno pointer */
static int *initialize_errno(void);
int *(*wince_errno_pointer_function)(void) = initialize_errno;

/* Static errno storage for the main thread */
static int errno_storage = 0;

/* Thread-Local storage slot for errno */
static int tls_errno_slot = 0xffffffff;

/*
 * Number of threads we have running and critical section protection
 * for manipulating it
 */
static int number_of_threads = 0;
static CRITICAL_SECTION number_of_threads_critical_section;

/* For the main thread only -- return the errno pointer */
static int *
get_main_thread_errno(void)
{
    return &errno_storage;
}

/* When there is more than one thread -- return the errno pointer */
static int *
get_thread_errno(void)
{
    return (int *)TlsGetValue(tls_errno_slot);
}

/* Initialize a thread's errno */
static void
initialize_thread_errno(int *errno_pointer)
{
    /* Make sure we have a slot */
    if (tls_errno_slot == 0xffffffff) {
        /* No: Get one */
        tls_errno_slot = (int)TlsAlloc();
        if (tls_errno_slot == 0xffffffff)
            abort();
    }
    /*
     * We can safely check for 0 threads, because
     * only the main thread will be initializing
     * at this point.  Make sure the critical
     * section that protects the number of threads
     * is initialized
     */
    if (number_of_threads == 0)
        InitializeCriticalSection(&number_of_threads_critical_section);
    /* Store the errno pointer */
    if (TlsSetValue(tls_errno_slot, (LPVOID)errno_pointer) == 0)
        abort();
    /* Bump the number of threads */
    EnterCriticalSection(&number_of_threads_critical_section);
    number_of_threads++;
    if (number_of_threads > 1) {
        /*
         * We have threads other than the main thread:
         *   Use thread-local storage
         */
        wince_errno_pointer_function = get_thread_errno;
    }
    LeaveCriticalSection(&number_of_threads_critical_section);
}

/* Initialize errno emulation on Windows CE (Main thread) */
static int *
initialize_errno(void)
{
    /* Initialize the main thread's errno in thread-local storage */
    initialize_thread_errno(&errno_storage);
    /*
     * Set the errno function to be the one that returns the
     * main thread's errno
     */
    wince_errno_pointer_function = get_main_thread_errno;
    /*
     *	Return the main thread's errno
     */
    return &errno_storage;
}

/* Initialize errno emulation on Windows CE (New thread) */
void
wince_errno_new_thread(int *errno_pointer)
{
    initialize_thread_errno(errno_pointer);
}

/* Note that a thread has exited */
void
wince_errno_thread_exit(void)
{
    /* Decrease the number of threads */
    EnterCriticalSection(&number_of_threads_critical_section);
    number_of_threads--;
    if (number_of_threads <= 1) {
        /* We only have the main thread */
        wince_errno_pointer_function = get_main_thread_errno;
    }
    LeaveCriticalSection(&number_of_threads_critical_section);
}

char *
strerror(int errnum)
{
    if (errnum >= sizeof(_sys_errlist)) {
        errno = ECONVERT;
        errnum = errno;
    }
    return _sys_errlist[errnum];
}

DWORD
FormatMessageA(DWORD flags, const void *source, DWORD msg, DWORD lang, char *buf, DWORD buf_size,
               va_list *args)
{
    if (flags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        void *pwbuf;
        void **ppbuf = (void **)buf;
        DWORD n;
        DWORD ret = FormatMessageW(flags, source, msg, lang, (WCHAR *)&pwbuf, buf_size, args);
        if (ret == 0)
            return 0;
        n = WideCharToMultiByte(CP_ACP, 0, pwbuf, -1, NULL, 0, NULL, NULL);
        *ppbuf = LocalAlloc(LMEM_FIXED, n);
        if (*ppbuf == NULL)
            return 0;
        ret = WideCharToMultiByte(CP_ACP, 0, pwbuf, -1, *ppbuf, n, NULL, NULL);
        LocalFree(pwbuf);
        return ret;
    }
    else {
        WCHAR wbuf[256];
        DWORD ret =
            FormatMessageW(flags, source, msg, lang, wbuf, sizeof(wbuf) / sizeof(wbuf[0]), NULL);
        return WideCharToMultiByte(CP_ACP, 0, wbuf, -1, buf, buf_size, NULL, NULL);
    }
}

char *
setlocale(int category, const char *locale) {
	if (locale==NULL) {
		return "C";
	}
	if (strcmp(locale, "C") != 0) {
		return NULL;
	}
	return "C";
}

static WCHAR default_current_directory[] = L"\\Temp";
static WCHAR *current_directory = default_current_directory;

#define ISSLASH(c) ((c) == L'\\' || (c) == L'/')

/*
 * WinCE has more restrictive path handling than Windows NT so we
 * need to do some conversions for compatibility
 */
static void
wince_canonicalize_path(WCHAR *path)
{
    WCHAR *p = path;

    /* Replace forward slashes with backslashes */
    while (*p) {
        if (*p == L'/')
            *p = L'\\';
        p++;
    }
    /* Strip \. and \.. from the beginning of absolute paths */
    p = path;
    while (p[0] == L'\\' && p[1] == L'.') {
        if (p[2] == L'.') {
            /* Skip \.. */
            p += 3;
        }
        else {
            /* Skip \. */
            p += 2;
        }
    }
    if (!*p) {
        /* If we stripped everything then return the root \
         * instead of an empty path
         */
        wcscpy(path, L"\\");
    }
    else if (p != path) {
        /* We skipped something so we need to delete it */
        int size = (wcslen(p) + 1) * sizeof(WCHAR);
        memmove(path, p, size);
    }
}

/* Converts a wide relative path to a wide absolute path */
void
wince_absolute_path_wide(const WCHAR *path, WCHAR *buffer, int buffer_size)
{
    int path_length, directory_length;
    WCHAR *cp;
    WCHAR *cp1;

    /* Check for a path that is already absolute */
    if (ISSLASH(*path)) {
        /* Yes: Just return it */
        path_length = wcslen(path);
        if (path_length >= buffer_size)
            path_length = buffer_size - 1;
        memcpy(buffer, path, path_length * sizeof(WCHAR));
        buffer[path_length] = L'\0';
        wince_canonicalize_path(buffer);
        return;
    }

    /* Need to turn it into an absolute path: strip "." and ".." */
    cp = (WCHAR *)path;
    /* cp1 points to the null terminator */
    cp1 = current_directory;
    while (*cp1) cp1++;
    while (*cp == L'.') {
        if (ISSLASH(cp[1])) {
            /* Strip ".\\" from beginning */
            cp += 2;
            continue;
        }
        if (cp[1] == L'\0') {
            /* "." is the same as no path */
            cp++;
            break;
        }
        /* Handle filenames like ".abc" */
        if (cp[1] != L'.')
            break;
        /* Handle filenames like "..abc" */
        if (!ISSLASH(cp[2]) && (cp[2] != L'\0'))
            break;
        /* Skip ".." */
        cp += 2;
        /* Skip backslash following ".." */
        if (*cp)
            cp++;
        /* Find the final backslash in the current directory path */
        while (cp1 > current_directory)
            if (ISSLASH(*--cp1))
                break;
    }

    /*
     * Now look for device specifications (and get the length of the path)
     */
    path = cp;
    while (*cp) {
        if (*cp++ == L':') {
            /* Device: Just return it */
            path_length = wcslen(path);
            if (path_length >= buffer_size)
                path_length = buffer_size - 1;
            memcpy(buffer, path, path_length * sizeof(WCHAR));
            buffer[path_length] = L'\0';
            wince_canonicalize_path(buffer);
            return;
        }
    }
    path_length = cp - path;
    /* Trim off trailing backslash */
    if ((path_length > 0) && (ISSLASH(cp[-1])))
        path_length--;
    /* If we backed up past the root, we are at the root */
    directory_length = cp1 - current_directory;
    if (directory_length == 0)
        directory_length = 1;
    /*
     * Output the directory and the path separator (truncated as necessary)
     */
    buffer_size -= 2; /* Account for the null terminator and the path separator */
    if (directory_length >= buffer_size)
        directory_length = buffer_size;
    memcpy(buffer, current_directory, directory_length * sizeof(WCHAR));
    buffer[directory_length] = L'\\';
    buffer_size -= directory_length;
    /* If there is no path, make sure we remove the path separator */
    if (path_length == 0)
        directory_length--;
    /* Output the path (truncated as necessary) */
    if (path_length >= buffer_size)
        path_length = buffer_size;
    memcpy(buffer + directory_length + 1, path, path_length * sizeof(WCHAR));
    buffer[directory_length + 1 + path_length] = '\0';
    wince_canonicalize_path(buffer);
}

#define FT_EPOCH (116444736000000000LL)
#define FT_TICKS (10000000LL)

/* Convert a Windows FILETIME to a time_t */
static time_t
convert_FILETIME_to_time_t(FILETIME *ft)
{
    __int64 temp;

    /*
     * Convert the FILETIME structure to 100nSecs since 1601 (as a 64-bit value)
     */
    temp = (((__int64)ft->dwHighDateTime) << 32) + (__int64)ft->dwLowDateTime;
    /* Convert to seconds from 1970 */
    return ((time_t)((temp - FT_EPOCH) / FT_TICKS));
}

int
_wstat(const wchar_t *path, struct wince__stat *buffer)
{
    WIN32_FIND_DATA data;
    HANDLE hFile;
    WCHAR *p;
    unsigned attr;
    WCHAR absolute_path[MAX_PATH + 1];

    /* Get the absolute path */
    wince_absolute_path_wide(path, absolute_path, MAX_PATH + 1);

    /* Get the file attributes */
    attr = GetFileAttributesW(absolute_path);
    if (attr == 0xFFFFFFFF) {
        errno = GetLastError();
        return -1;
    }

    /* Check for stuff we can't deal with */
    p = absolute_path;
    while (*p) {
        if ((*p == L'?') || (*p == L'*'))
            return -1;
        p++;
    }
    hFile = FindFirstFile(absolute_path, &data);
    if (hFile == INVALID_HANDLE_VALUE) {
        errno = GetLastError();
        return -1;
    }
    FindClose(hFile);

    /* Found: Convert the file times */
    buffer->st_mtime = convert_FILETIME_to_time_t(&data.ftLastWriteTime);
    if (data.ftLastAccessTime.dwLowDateTime || data.ftLastAccessTime.dwHighDateTime)
        buffer->st_atime = convert_FILETIME_to_time_t(&data.ftLastAccessTime);
    else
        buffer->st_atime = buffer->st_mtime;
    if (data.ftCreationTime.dwLowDateTime || data.ftCreationTime.dwHighDateTime)
        buffer->st_ctime = convert_FILETIME_to_time_t(&data.ftCreationTime);
    else
        buffer->st_ctime = buffer->st_mtime;

    /* Convert the file attributes */
    if (absolute_path[1] == L':')
        p += 2;
    buffer->st_mode = (unsigned short)(((ISSLASH(*p) && !absolute_path[1]) ||
                                        (attr & FILE_ATTRIBUTE_DIRECTORY) || *p)
                                           ? S_IFDIR | S_IEXEC
                                           : S_IFREG);
    buffer->st_mode |= (attr & FILE_ATTRIBUTE_READONLY) ? S_IREAD : (S_IREAD | S_IWRITE);
    p = absolute_path + wcslen(absolute_path);
    while (p >= absolute_path) {
        if (*--p == L'.') {
            if (p[1] && ((p[1] == L'e') || (p[1] == L'c') || (p[1] == L'b')) && p[2] &&
                ((p[2] == L'x') || (p[2] == L'm') || (p[2] == L'a') || (p[2] == L'o')) && p[3] &&
                ((p[3] == L'e') || (p[3] == L'd') || (p[3] == L't') || (p[3] == L'm')))
                buffer->st_mode |= S_IEXEC;
            break;
        }
    }
    buffer->st_mode |= (buffer->st_mode & 0700) >> 3;
    buffer->st_mode |= (buffer->st_mode & 0700) >> 6;
    /* Set the other information */
    buffer->st_nlink = 1;
    buffer->st_size = (unsigned long int)data.nFileSizeLow;
    buffer->st_uid = 0;
    buffer->st_gid = 0;
    buffer->st_ino = 0;
    buffer->st_dev = 0;
    /* Return success */
    return 0;
}

DWORD
GetCurrentDirectoryW(DWORD numbuf, wchar_t *buffer)
{
    DWORD n = wcslen(current_directory) + 1;
    if (n > numbuf)
        return n;
    memcpy(buffer, current_directory, n * sizeof(wchar_t));
    return n - 1;
}

BOOL
SetCurrentDirectoryW(const WCHAR *path)
{
    int length;
    WCHAR *cp;
    WCHAR absolute_path[MAX_PATH + 1];
    DWORD attr;

    /* Get the absolute path */
    wince_absolute_path_wide(path, absolute_path, MAX_PATH + 1);
    /* Make sure it exists */
    attr = GetFileAttributesW(absolute_path);
    if (attr == 0xFFFFFFFF)
        return FALSE;
    if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* Put it into dynamic memory */
    length = wcslen(absolute_path);
    cp = (WCHAR *)LocalAlloc(0, (length + 1) * sizeof(WCHAR));
    if (!cp)
        return FALSE;
    memcpy(cp, absolute_path, length * sizeof(WCHAR));
    cp[length] = L'\0';
    /* Free up any old allocation and store the new current directory */
    if (current_directory != default_current_directory)
        LocalFree(current_directory);
    current_directory = cp;
    return TRUE;
}

DWORD
GetFullPathNameW(const wchar_t *path, DWORD num_buf, wchar_t *buf, wchar_t **file_part)
{
    if (path == NULL || wcslen(path) == 0) {
        SetLastError(123); // ERROR_INVALID_NAME
        return 0;
    }

    wince_absolute_path_wide(path, buf, num_buf);
    if (file_part) {
        *file_part = wcsrchr(buf, '\\');
        if (*file_part)
            (*file_part)++;
        else
            *file_part = buf;
    }
    return wcslen(buf);
}

char *
wince_asctime(const struct tm *tm)
{
    WCHAR temp[26];
    static const char *Days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static char buffer[26];

    /* Format the string */
    wsprintfW(temp, TEXT("%hs %hs %2d %02d:%02d:%02d %4d\n"), Days[tm->tm_wday], Months[tm->tm_mon],
              tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year + 1900);
    /* Convert to ascii and return it */
    WideCharToMultiByte(CP_ACP, 0, temp, sizeof(temp) / sizeof(temp[0]), buffer,
                        sizeof(buffer) / sizeof(buffer[0]), NULL, NULL);
    return buffer;
}

clock_t
clock(void)
{
    /* Not supported */
    return (clock_t)-1;
}

#undef DeleteFileW
BOOL
wince_DeleteFileW(const wchar_t *path)
{
    WCHAR absolute_path[MAX_PATH + 1];
    wince_absolute_path_wide(path, absolute_path, MAX_PATH + 1);
    return DeleteFileW(absolute_path);
}

BOOL
DeleteFileA(const char *path)
{
    WCHAR wpath[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, sizeof(wpath) / sizeof(wpath[0]));
    return wince_DeleteFileW(wpath);
}

#undef MoveFileW
BOOL
wince_MoveFileW(const wchar_t *oldpath, const wchar_t *newpath)
{
    WCHAR absolute_path1[MAX_PATH + 1];
    WCHAR absolute_path2[MAX_PATH + 1];
    wince_absolute_path_wide(oldpath, absolute_path1, MAX_PATH + 1);
    wince_absolute_path_wide(newpath, absolute_path2, MAX_PATH + 1);
    return MoveFileW(absolute_path1, absolute_path2);
}

BOOL
MoveFileA(const char *oldpath, const char *newpath)
{
    WCHAR woldpath[MAX_PATH + 1];
    WCHAR wnewpath[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, oldpath, -1, woldpath, sizeof(woldpath) / sizeof(woldpath[0]));
    MultiByteToWideChar(CP_ACP, 0, newpath, -1, wnewpath, sizeof(wnewpath) / sizeof(wnewpath[0]));
    return wince_MoveFileW(woldpath, wnewpath);
}

wchar_t **
CommandLineToArgvW(const wchar_t *lpCmdLine, int *pNumArgs)
{
    wchar_t **argv;
    int argvSize = 64;
    argv = (wchar_t **)calloc(argvSize, sizeof(wchar_t *));
    if (argv == NULL)
        return NULL;

    int argc = 1;
    int i = -1;
    int i2 = 0;
    int quoted = 0;
    int backslash = 0;
    int spaced = 1;
    int error = 0;

    wchar_t exeName[MAX_PATH + 1];

    int argTmpSize = 64;

    int cmdlen = (int)wcslen(lpCmdLine);

    wchar_t *curChar = lpCmdLine;

    wchar_t *argTmp;
    wchar_t *argTmpOrg;
    argTmpOrg = (wchar_t *)calloc(argTmpSize, sizeof(wchar_t));
    argTmp = argTmpOrg;

    if (argTmp == NULL) {
        free(argv);
        return NULL;
    }

    GetModuleFileName(NULL, exeName, MAX_PATH + 1);
    argv[0] = (wchar_t *)calloc(wcslen(exeName)+1, sizeof(wchar_t));
    if (argv[0] == NULL) {
        free(argv);
        free(argTmpOrg);
        return NULL;
    }
    wcscpy(argv[0], exeName);

    while (i < cmdlen) {
        if (i < 0 && cmdlen == 0)
            break;
        i++;
        if (spaced && *curChar != L' ' && *curChar != L'\t' && *curChar != L'\0') {
            spaced = 0;
            i2 = 0;
            argc++;
        } else if (spaced) {
            curChar++;
            continue;
        }
        if (i2 >= argTmpSize) {
            argTmpSize += 16;
            wchar_t *tmp = (wchar_t *)realloc(argTmpOrg, argTmpSize * sizeof(wchar_t));
            if (tmp == NULL) {
                error = 1;
                break;
            }
            argTmp = tmp + (argTmp - argTmpOrg);
            argTmpOrg = tmp;
        }
        if (*curChar == L'\\') {
            backslash++;
            curChar++;
            continue;
        }
        if (*curChar == L'"') {
            if (i2 + (backslash + 1) / 2 >= argTmpSize) {
                argTmpSize += 16 * ((backslash + 1) / 2);
                wchar_t *tmp = (wchar_t *)realloc(argTmpOrg, argTmpSize * sizeof(wchar_t));
                if (tmp == NULL) {
                    error = 1;
                    break;
                }
                argTmp = tmp + (argTmp - argTmpOrg);
                argTmpOrg = tmp;
            }
            for (int index = 0; index < backslash; index += 2) {
                *argTmp = L'\\';
                backslash -= 2;
                argTmp++;
                i2++;
            }
            if (backslash) {
                *argTmp = L'"';
                argTmp++;
                i2++;
            }
            if (quoted)
                quoted = backslash;
            else
                quoted = 1 - backslash;
            backslash = 0;
            curChar++;
            continue;
        }
        if (backslash) {
            for (int index = 0; index < backslash; index++) {
                *argTmp = L'\\';
                argTmp++;
                i2++;
            }
            backslash = 0;
        }
        if (!spaced && !quoted && (*curChar == L' ' || *curChar == L'\t' || *curChar == L'\0')) {
            argv[argc - 1] = (wchar_t *)calloc(i2 + 1, sizeof(wchar_t));
            if (argv[argc - 1] == NULL) {
                error = 1;
                break;
            }
            wcsncpy(argv[argc - 1], argTmpOrg, i2);
            if (argTmpSize > 64) {
                argTmpSize = 64;
                free(argTmpOrg);
                argTmpOrg = (wchar_t *)calloc(argTmpSize, sizeof(wchar_t));
                if (argTmpOrg == NULL) {
                    error = 1;
                    break;
                }
            }
            else {
                wmemset(argTmpOrg, L'\0', i2);
            }
            argTmp = argTmpOrg;
            spaced = 1;
            curChar++;
            if (argvSize >= argc) {
                argvSize += 16;
                argv = (wchar_t **)realloc(argv, argvSize * sizeof(wchar_t *));
                if (argv == NULL) {
                    free(argTmpOrg);
                    error = 1;
                    break;
                }
            }
            continue;
        }
        *argTmp = *curChar;
        argTmp++;
        if (*curChar == L'\0')
            break;
        curChar++;
        i2++;
    }
    if (error) {
        free(argv);
        free(argTmpOrg);
        return NULL;
    }

    wchar_t **result = (wchar_t **)realloc(argv, sizeof(wchar_t *) * (argc + 1));
    if (result != NULL) {
        *pNumArgs = argc;
    }
    else {
        free(argv);
    }
    free(argTmpOrg);
    result[argc] = 0;
    return result;
}
