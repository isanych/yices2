#pragma once

#include <stddef.h>
#include <string.h>

#ifdef _WIN32
/*
 * We call isatty(STDIN_FILENO) to check whether stdin is a terminal.
 *
 * On Windows/MINGW, isatty is called _isatty. The macro STDIN_FILENO
 * appears to be defined in mingw/include/stdio.h. Not clear whether
 * it exists in Windows?  There is a function isatty declared in platform.h,
 * but it is deprecated.
 *
 * NOTE: the windows function _isatty doesn't have the same behavior
 * as isatty on Unix. It returns a non-zero value if the file
 * descriptor is associated with a character device (which is true of
 * terminals but of other files too).
 */
#include <io.h>
#ifndef STDIN_FILENO
#define STDIN_FILENO (_fileno(stdin))
#endif
#define isatty _isatty
#define strtok_r strtok_s
#define ssize_t ptrdiff_t
#else
#include <unistd.h>
#endif
