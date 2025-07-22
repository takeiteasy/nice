/* pat.h -- https://github.com/takeiteasy/pat

 Copyright (C) 2024  George Watson

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>. */

/*!
 @header pat.h
 @copyright George Watson GPLv3
 @updated 2025-07-19
 @abstract Cross-platform file and path utilities for C/C++.
 @discussion Supports Windows, macOS and Linux/Unix-like systems.
             Provides file I/O, directory management, path manipulation, and globbing.
             Designed to be used in C/C++ projects with C17 standard.
 */

#ifndef PAT_H
#define PAT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __has_include
#define __has_include(x) 0
#endif
#ifndef __has_extension
#define __has_extension(x) 0
#endif

#if __has_include(<sys/param.h>)
#include <sys/param.h>
#endif

#ifdef __EMSCRIPTEN__
#define PAT_NO_NATIVE
#endif

#ifdef PAT_NO_NATIVE
#define PLATFORM_UNKNOWN
#elif (defined(PAT_NO_COCOA) && defined(PLATFORM_MACOS)) || \
       defined(PAT_FORCE_POSIX)
#define PLATFORM_NIX
#else
#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM_MACOS
#elif defined(__linux__) || defined(__LINUX__) || \
      defined(__unix__)  || defined(__UNUX__)  || \
      defined(BSD)       || defined(__BSD__)
#define PLATFORM_NIX
#else
#define PLATFORM_UNKNOWN
#endif
#endif

#if defined(PLATFORM_MACOS) || defined(PLATFORM_NIX)
#define PLATFORM_POSIX
#endif

#if defined(__clang__) || defined(__GNUC__)
#if __has_extension(blocks)
#define PLATFORM_HAS_BLOCKS
#if __has_include(<Block.h>)
#include <Block.h>
#endif
#endif
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#elif defined(PLATFORM_POSIX)
#include <unistd.h>
#include <dirent.h>
#else
#include <stdio.h>
#endif

#ifndef MAX_PATH
#if defined(PLATFORM_MACOS)
#define MAX_PATH 255
#elif defined(PLATFORM_WINDOWS)
#define MAX_PATH 256
#elif defined(PLATFORM_NIX)
#define MAX_PATH 4096
#else
#define MAX_PATH 1024
#endif
#endif

#ifdef PLATFORM_WINDOWS
#define PATH_SEPERATOR '\\'
#define PATH_SEPERATOR_STR "\\"
#else
#define PATH_SEPERATOR '/'
#define PATH_SEPERATOR_STR "/"
#endif

#ifdef PLATFORM_WINDOWS
#define IO_IN STD_INPUT_HANDLE
#define IO_OUT STD_OUTPUT_HANDLE
#define IO_ERR STD_ERROR_HANDLE
#else
#define IO_IN STDIN_FILENO
#define IO_OUT STDOUT_FILENO
#define IO_ERR STDERR_FILENO
#endif

/*!
 @typedef file_t
 @field fd Underlying file handler
 @discussion file_t is a wrapper around OS specific file handlers
 */
typedef struct file {
#ifdef PLATFORM_WINDOWS
    HANDLE fd;
#elif defined(PLATFORM_POSIX)
    int fd;
#else
    FILE* fd;
#endif
} file_t;

#ifdef PLATFORM_WINDOWS
#define INVALID_FILE_HANDLE INVALID_HANDLE_VALUE
#else
#define INVALID_FILE_HANDLE -1
#endif

/*!
 @const io_invalid
 @discussion Invalid file handler (-1)
 */
extern const file_t io_invalid;
/*!
 @const io_out
 @discussion Standard out
 */
extern const file_t io_out;
/*!
 @const io_err
 @discussion Standard error
 */
extern const file_t io_err;
/*!
 @const io_in
 @discussion Standard in
 */
extern const file_t io_in;

/*!
 @function io_print
 @param IO File stream to write to
 @param MSG Message to print
 @abstract Print a message to a file stream
 */
#define io_print(IO, MSG)             \
    do                                \
    {                                 \
        if (io_valid(IO))             \
            io_write_string(IO, MSG); \
    } while (0)
/*!
 @function io_println
 @param IO File stream to write to
 @param MSG Message to print
 @abstract Print a message to a file stream with a newline
 */
#define io_println(IO, MSG)     \
    do                          \
    {                           \
        io_print(IO, MSG);      \
        io_print(IO, "\n")      \
    } while(0)
/*!
 @function print
 @param MSG Message to print
 @abstract Print a message to standard out
 */
#define print(MSG)       io_print(io_out, MSG)
/*!
 @function println
 @param MSG Message to print
 @abstract Print a message to standard out with a newline
 */
#define println(MSG)     io_println(io_out, MSG)
/*!
 @function print_err
 @param MSG Message to print
 @abstract Print a message to standard error
 */
#define print_err(MSG)   io_print(io_err, MSG)
/*!
 @function println_err
 @param MSG Message to print
 @abstract Print a message to standard error with a newline
 */
#define println_err(MSG) io_println(io_err, MSG)

/*!
 @enum file_mode_t
 @constant FILE_READ File read flag (r)
 @constant FILE_WRITE File write flag (w)
 @constant FILE_APPEND File append flag (a)
 @discussion File permissions for io_open
 */
enum file_mode_t {
    FILE_READ   = 0x00000001,
    FILE_WRITE  = 0x00000002,
    FILE_APPEND = 0x00000004
};

/*!
 @enum file_seek_t
 @constant FILE_START Beginning of file marker (SEEK_SET)
 @constant FILE_CURSOR Current file position marker (SEEK_CUR)
 @constant FILE_FINISH End of file marker (SEEK_END)
 @discussion File offset positions for io_seek
 */
enum file_seek_t {
    FILE_START  = 0,
    FILE_CURSOR,
    FILE_FINISH
};

/*!
 @function io_open
 @param dst File steam destination
 @param path Path to file
 @param mode File steam access mode(s)
 @abstract Open file stream
 */
bool io_open(file_t *dst, const char* path, enum file_mode_t mode);
/*!
 @function io_close
 @param file Pointer to file steam to close
 @return Returns true/false on success
 @abstract Close file stream
 */
bool io_close(file_t *file);
/*!
 @function io_read
 @param file File stream to read from
 @param buffer Buffer to read to
 @param size Size of the destination buffer
 @return Returns true/false on success
 @abstract Read n bytes from file stream
 @discussion Please ensure that size >= destination buffer size
 */
size_t io_read(file_t file, void *buffer, size_t size);
/*!
 @function io_write
 @param file File stream to write to
 @param buffer Source buffer to write
 @param size Number of bytes to write
 @return Returns number of bytes read, -1 on error
 @abstract Write n bytes to file stream
 @discussion Please ensure that size is >= the source buffer size
 */
size_t io_write(file_t file, const void *buffer, size_t size);
/*!
 @function io_seek
 @param file File stream to modify
 @param offset The number of bytes to seek
 @param whence The offset of the seek
 @return Returns true/false on success
 @abstract Seek file stream position
 */
bool io_seek(file_t file, long offset, enum file_seek_t whence);
/*!
 @function io_advance
 @param file File stream to modify
 @param offset Number of bytes to seek
 @return Returns true/false on success
 @abstract Advance the file stream cursor by n bytes
 */
bool io_advance(file_t file, long offset);
/*!
 @function io_tell
 @param file File stream to check
 @return Returns the current file stream position, -1 on error
 @abstract Get current position of file stream
 */
size_t io_tell(file_t file);
/*!
 @function io_eof
 @param file File stream to check
 @return Returns true is file is at the end
 @abstract Check if file is at the end
 */
bool io_eof(file_t file);
/*!
 @function io_flush
 @param file File stream to flush
 @return Returns true/false on success
 @abstract Flush file stream
 */
bool io_flush(file_t file);
/*!
 @function io_valid
 @param file File stream to check
 @return Check if a file stream is valid
 @abstract Check if a file stream is valid
 */
bool io_valid(file_t file);
/*!
 @function io_read_line
 @param file File stream to read
 @param buffer Destination buffer
 @param size Size of destination buffer
 @return Returns true/false on success
 @abstract Read until newline or end of file
 */
bool io_read_line(file_t file, char *buffer, size_t size);
/*!
 @function io_write_string
 @param file File stream to write to
 @param str String to write
 @return Returns true/false on success
 @abstract Write a string to a file stream
 */
bool io_write_string(file_t file, const char *str);
/*!
 @function io_truncate
 @param file File stream to modify
 @param size Offset bytes
 @return Returns true/false on success
 @abstract Truncate a file stream to a specific length
 */
bool io_truncate(file_t file, long size);

/*!
 @function enviroment_variable
 @param name Variable name
 @return Returns enviroment variable string or NULL on failure
 @abstract Get an enviroment variable
 @discussion Return value will need to be freed on success
 */
char* enviroment_variable(const char *name); // !

/*!
 @function file_exists
 @param path Path to file
 @return Returns true if file exists
 @abstract Check if a file exists
 @discussion Checks if a path exists and is a file (directory will return false)
 */
bool file_exists(const char *path);
/*!
 @function file_delete
 @param path Path to file
 @return Returns true/false on success
 @abstract Delete a file from file system
 */
bool file_delete(const char *path);
/*!
 @function file_rename
 @param old_path Path to file to rename
 @param new_path New path for file
 @param write_over Write over existing files when moving
 @return Returns true/false on success
 @abstract Move a file
 @discussion WIP
 */
bool file_rename(const char *old_path, const char *new_path, bool write_over);
/*!
 @function file_copy
 @param src_path Path to original file
 @param dst_path Path to copy
 @param write_over Write over existing files when moving
 @return Returns true/false on success
 @abstract Copy a file
 @discussion WIP
 */
bool file_copy(const char *src_path, const char *dst_path, bool write_over);
/*!
 @function file_size
 @param path Path to file
 @return Returns size of a file, -1 on error
 @abstract Get file size
 */
int file_size(const char *path);
/*!
 @function file_read
 @param path Path to file
 @param size Pointer to recieve file size
 @return Returns the contents of a file
 @abstract Read a file from disk
 @discussion Return value will need to be freed on success
 */
const char* file_read(const char *path, size_t *size);  // !

/*!
 @function directory_exists
 @param path Path to directory
 @return Returns true if file exists
 @abstract Check if a directory exists
 @discussion Checks if a path exists and is a directory (file will return false)
 */
bool directory_exists(const char *path);
/*!
 @function directory_create
 @param path Path to directory
 @param recursive If this is true, will create any required parent directories
 @return Returns true/false on success
 @abstract Create a directory
 @discussion WIP
 */
bool directory_create(const char *path, bool recursive);
/*!
 @function directory_delete
 @param path Path to directory
 @param recursive If this is true, it will delete and child directories too
 @param and_files If this is ture, it will delete and files inside the directory
 @return Returns true/false on success
 @abstract Delete a directory
 @discussion WIP
 */
bool directory_delete(const char *path, bool recursive, bool and_files);
/*!
 @function directory_rename
 @param old_path Path to directory
 @param new_path Path to directory to move to
 @param write_over If this is true, write over any existing files when copying
 @return Returns true/false on success
 @abstract Move a directory
 @discussion WIP
 */
bool directory_rename(const char *old_path, const char *new_path, bool write_over);
/*!
 @function directory_copy
 @param src_path Path to directory
 @param dst_path Path copy directory to
 @param write_over If this is true, write over any existing files when copying
 @param delete_src If this is true, delete the original directory and files after copy succeeds
 @return Returns true/false on success
 @abstract Copy a directory and contents
 */
bool directory_copy(const char *src_path, const char *dst_path, bool write_over, bool delete_src);
/*!
 @function directory_size
 @param path Path to directory
 @return Returns size of directory contents, -1 on error
 @abstract Get the size of a directory
 */
int directory_size(const char *path);
/*!
 @function directory_item_count
 @param path Path to directory
 @param recursive If this is true, child directories will also be counted
 @return Returns number of items in directory, -1 on error
 @abstract Get the number of files/directories inside a directory
 */
int directory_item_count(const char *path, bool recursive);
/*!
 @function directory_file_count
 @param path Path to directory
 @param recursive If this is true, child directories will also be counted
 @return Returns number of files in directory, -1 on error
 @abstract Get the number of files inside a directory
 @discussion This doesn't count directories
 */
int directory_file_count(const char *path, bool recursive);
/*!
 @function directory_glob
 @param pattern Glob pattern to match
 @param count Pointer to an integer to store the number of matches
 @param recursive If this is true, it will search recursively
 @return Returns an array of strings containing the matched file paths, or NULL on error
 @abstract Glob a directory for files matching a pattern
 @discussion The result must be freed by the caller.
             The count will be set to the number of matches.
             The array and its elements must be freed by the caller.
 */
const char** directory_glob(const char *pattern, bool recursive, int *count); // !!

/*!
 @typedef dir_t
 @field path Path to directory
 @discussion dir_t is a wrapper around OS specific directory iterators
 */
typedef struct dir {
    const char *path;
#ifdef PLATFORM_WINDOWS
    WIN32_FIND_DATA findData;
    HANDLE hFind;
#elif defined(PLATFORM_POSIX)
    DIR *dir;
#endif
} dir_t;

/*!
 @function directory_iter
 @param dir Pointer to dir_t structure
 @param is_dir Pointer to bool to store if the current item is a directory
 @return Returns the name of the next item in the directory, or NULL if there are no more items
 @abstract Iterate through a directory
 @discussion If you need to exit an iteration early, call directory_iter_end(dir)
 */
const char* directory_iter(dir_t *dir, bool *is_dir);
/*!
 @function directory_iter_end
 @param dir Pointer to dir_t structure
 @abstract End a directory iteration prematurely
 @discussion This will close the directory iterator and free any resources used.
             Normally this will be done at the end of the iteration.
 */
void directory_iter_end(dir_t *dir);

/*!
 @typedef glob_callback
 @param pattern The glob pattern
 @param filename The filename that matches the pattern
 @param userdata User data pointer
 @return Returns 0 to continue iteration, non-zero to stop
 */
typedef int(*glob_callback)(const char* pattern, const char* filename, void* userdata);
/*!
 @typedef walk_callback
 @param path The current path being walked
 @param filename The filename in the current path
 @param userdata User data pointer
 @return Returns 0 to continue walking, non-zero to stop
 */
typedef int(*walk_callback)(const char* path, const char* filename, void* userdata);
/*!
 @function path_glob
 @param pattern The glob pattern to match
 @param recursive If this is true, it will search recursively
 @param callback Callback function to call for each match
 @param userdata User data pointer to pass to the callback
 @return Returns true/false on success
 @abstract Glob a pattern and call a callback for each match
 */
bool path_glob(const char *pattern, bool recursive, glob_callback callback, void *userdata);
/*!
 @function path_walk
 @param path The path to walk
 @param recursive If this is true, it will walk recursively
 @param callback Callback function to call for each file/directory
 @param userdata User data pointer to pass to the callback
 @return Returns true/false on success
 @abstract Walk a directory and call a callback for each file/directory
 */
bool path_walk(const char *path, bool recursive, walk_callback callback, void *userdata);

#ifdef PLATFORM_HAS_BLOCKS
/*!
 @typedef glob_block
 @param pattern The glob pattern
 @param filename The filename that matches the pattern
 @param userdata User data pointer
 @return Returns 0 to continue iteration, non-zero to stop
 @discussion This is a block version of glob_callback. Blocks are a clang/gcc extension. -fblocks must be enabled to use this.
 */
typedef int(^glob_block)(const char* pattern, const char* filename, void* userdata);
/*!
 @typedef walk_block
 @param path The current path being walked
 @param filename The filename in the current path
 @param userdata User data pointer
 @return Returns 0 to continue walking, non-zero to stop
 @discussion This is a block version of walk_callback. Blocks are a clang/gcc extension. -fblocks must be enabled to use this.
 */
typedef int(^walk_block)(const char* path, const char* filename, void* userdata);
/*!
 @function path_glob_block
 @param pattern The glob pattern to match
 @param recursive If this is true, it will search recursively
 @param callback Block to call for each match
 @param userdata User data pointer to pass to the block
 @return Returns true/false on success
 @abstract Glob a pattern and call a block for each match
 @discussion Blocks are a clang/gcc extension. -fblocks must be enabled to use this.
 */
bool path_glob_block(const char *pattern, bool recursive, glob_block callback, void *userdata);
/*!
 @function path_walk_block
 @param path The path to walk
 @param recursive If this is true, it will walk recursively
 @param callback Block to call for each file/directory
 @param userdata User data pointer to pass to the block
 @return Returns true/false on success
 @abstract Walk a directory and call a block for each file/directory
 @discussion Blocks are a clang/gcc extension. -fblocks must be enabled to use this.
 */
bool path_walk_block(const char *path, bool recursive, walk_block callback, void *userdata);
#endif

/*!
 @function path_exists
 @param path The path to check
 @return Returns true/false on success
 @abstract Check if a path exists. This will return true for both files and directories.
 */
bool path_exists(const char *path);
/*!
 @function path_working_directory
 @return Returns the current working directory
 @abstract Get the current working directory
 */
const char* path_get_working_directory(void);
/*!
 @function path_set_working_directory
 @param path The path to set as the working directory
 @return Returns true/false on success
 @abstract Set the current working directory
 */
bool path_set_working_directory(const char *path);

/*!
 @function path_get_root_dir
 @return Returns the root directory
 @abstract Get the root directory
 */
const char* path_get_root_dir(void);
/*!
 @function path_get_home_dir
 @return Returns the home directory
 @abstract Get the home directory
 */
const char* path_get_home_dir(void);
/*!
 @function path_get_documents_dir
 @return Returns the documents directory
 @abstract Get the documents directory
 */
const char* path_get_documents_dir(void);
/*!
 @function path_get_downloads_dir
 @return Returns the downloads directory
 @abstract Get the downloads directory
 */
const char* path_get_downloads_dir(void);
/*!
 @function path_get_video_dir
 @return Returns the video directory
 @abstract Get the video directory
 */
const char* path_get_video_dir(void);
/*!
 @function path_get_music_dir
 @return Returns the music directory
 @abstract Get the music directory
 */
const char* path_get_music_dir(void);
/*!
 @function path_get_picture_dir
 @return Returns the picture directory
 @abstract Get the picture directory
 */
const char* path_get_picture_dir(void);
/*!
 @function path_get_application_dir
 @return Returns the application directory
 @abstract Get the application directory
 */
const char* path_get_application_dir(void);
/*!
 @function path_get_desktop_dir
 @return Returns the desktop directory
 @abstract Get the desktop directory
 */
const char* path_get_desktop_dir(void);

/*!
 @function path_get_file_extension
 @param path The path to get the file extension from
 @return Returns the file extension or NULL if there is no extension
 @abstract Get the file extension from a path
 */
const char* path_get_file_extension(const char *path);
/*!
 @function path_get_file_name
 @param path The path to get the file name from
 @return Returns the file name or NULL if there is no file name
 @abstract Get the file name from a path
 */
const char* path_get_file_name(const char *path);
/*!
 @function path_get_file_name_no_extension
 @param path The path to get the file name without extension from
 @return Returns the file name without extension or NULL if there is no file name
 @abstract Get the file name without extension from a path
 @discussion The result must be freed by the caller.
 */
const char* path_get_file_name_no_extension(const char *path);  // !
/*!
 @function path_without_file_name
 @param path The path to get the directory name from
 @return Returns the directory name or NULL if there is no directory name
 @abstract Get the directory name from a path
 @discussion The result must be freed by the caller.
 */
const char* path_without_file_name(const char *path);  // !
/*!
 @function path_get_directory_name
 @param path The path to get the directory name from
 @return Returns the directory name or NULL if there is no directory name
 @abstract Get the directory name from a path
 @discussion The result must be freed by the caller.
 */
const char* path_get_directory_name(const char *path); // !
/*!
 @function path_get_parent_directory
 @param path The path to get the parent directory from
 @return Returns the parent directory or NULL if there is no parent directory
 @abstract Get the parent directory from a path
 @discussion The result must be freed by the caller.
 */
const char* path_get_parent_directory(const char *path); // !

/*!
 @function path_resolve
 @param path The path to resolve
 @return Returns the resolved path or NULL on error
 @abstract Resolve a path to an absolute path
 @discussion This will attempt to resolve ~ to the user's home directory and ".." to the parent directory. The result must be freed by the caller. WIP.
 */
const char* path_resolve(const char *path); // !
/*!
 @function path_join
 @param a The first path
 @param b The second path
 @return Returns the joined path or NULL on error
 @abstract Join two paths together
 @discussion The result must be freed by the caller.
 */
const char* path_join(const char *a, const char *b); // !
/*!
 @function path_join_va
 @param n The number of paths to join
 @param ... The paths to join
 @return Returns the joined path or NULL on error
 @abstract Join multiple paths together
 @discussion The result must be freed by the caller.
 */
const char* path_join_va(int n, ...); // !
/*!
 @function path_split
 @param path The path to split
 @param count Pointer to an integer to store the number of parts
 @return Returns an array of strings containing the parts of the path, or NULL on error
 @abstract Split a path into its components
 @discussion The result must be freed by the caller.
             The count will be set to the number of parts.
             The array and its elements must be freed by the caller.
 */
const char** path_split(const char *path, size_t *count); // !!

#ifdef __cplusplus
}
#endif
#endif // PAT_H

#ifdef PAT_IMPLEMENTATION
#include <stdlib.h>
#include <string.h>

#ifdef PLATFORM_WINDOWS
#include <shlobj.h>
#include <objbase.h>
#include <combaseapi.h>
#include <KnownFolders.h>
#elif defined(PLATFORM_POSIX)
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
extern char **environ;
#endif

#ifdef PLATFORM_MACOS
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/NSObjCRuntime.h>
#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#endif

#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
#define NSIntegerEncoding "q"
#define NSUIntegerEncoding "L"
#else
#define NSIntegerEncoding "i"
#define NSUIntegerEncoding "I"
#endif

#ifdef __arm64__
#define abi_objc_msgSend_stret objc_msgSend
#else
#define abi_objc_msgSend_stret objc_msgSend_stret
#endif
#ifdef __i386__
#define abi_objc_msgSend_fpret objc_msgSend_fpret
#else
#define abi_objc_msgSend_fpret objc_msgSend
#endif

#define __STRINGIFY(s) #s

#define sel(NAME) sel_registerName(__STRINGIFY(NAME))
#define class(NAME) ((id)objc_getClass(__STRINGIFY(NAME)))
#define protocol(NAME) objc_getProtocol(__STRINGIFY(NAME))

#define ObjC(RET, ...) ((RET(*)(id, SEL, ##__VA_ARGS__))objc_msgSend)
#endif // PLATFORM_MACOS

const file_t io_invalid = {.fd=INVALID_FILE_HANDLE};

const file_t io_out = {.fd=IO_OUT};
const file_t io_err = {.fd=IO_ERR};
const file_t io_in  = {.fd=IO_IN};

bool io_open(file_t *dst, const char* path, enum file_mode_t mode) {
    if (!dst || !path)
        return false;
#ifdef PLATFORM_WINDOWS
    DWORD access = 0;
    DWORD creation = 0;
    if (mode & FILE_READ)
        access |= GENERIC_READ;
    if (mode & FILE_WRITE)
        access |= GENERIC_WRITE;
    if (mode & FILE_APPEND)
        access |= FILE_APPEND_DATA;
        if (!(mode & FILE_WRITE))
            access |= GENERIC_WRITE;
    }

    if ((mode & FILE_READ) && !(mode & (FILE_WRITE | FILE_APPEND)))
        creation = OPEN_EXISTING;
    else if ((mode & (FILE_WRITE | FILE_APPEND))) {
        creation = OPEN_ALWAYS & (((mode & FILE_WRITE) && !(mode & FILE_APPEND)) ? CREATE_ALWAYS : OPEN_ALWAYS);
    else
        return false;

    if ((dst->fd = CreateFileA(path, access,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               creation,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL)) == INVALID_FILE_HANDLE)
        return false;
    if (mode & FILE_APPEND)
        SetFilePointer(dst->fd, 0, NULL, FILE_END);
#elif defined(PLATFORM_POSIX)
    int flags = 0;
    mode_t file_permissions = 0644;
    if (mode & FILE_READ)
        flags |= mode & (FILE_WRITE | FILE_APPEND) ? O_RDWR : O_RDONLY;
    else if (mode & (FILE_WRITE | FILE_APPEND))
        flags |= O_WRONLY;
    else
        return false;

    if (mode & (FILE_WRITE | FILE_APPEND))
        flags |= O_CREAT;
    if (mode & FILE_WRITE && !(mode & FILE_APPEND))
        flags |= O_TRUNC;
    if (mode & FILE_APPEND)
        flags |= O_APPEND;
    if ((dst->fd = open(path, flags, file_permissions)) == INVALID_FILE_HANDLE)
        return false;
#else
    char buffer[3];
    if (mode & FILE_READ) {
        buffer[0] = 'r';
        buffer[1] = mode & FILE_APPEND ? '+' : '\0';
    } else if (mode & FILE_WRITE) {
        buffer[0] = 'w';
        buffer[1] = mode & FILE_APPEND ? '+' : '\0';
    } else if (mode & FILE_APPEND) {
        buffer[0] = 'a';
        buffer[1] = '\0';
    }
    buffer[2] = '\0';
    if (!(dst->fd = fopen(path, buffer)))
        return false;
#endif
    return true;
}


bool io_close(file_t *file) {
    if (!file || !io_valid(*file))
        return false;
    bool result = false;
#ifdef PLATFORM_WINDOWS
    result = CloseHandle(file->fd) != 0;
#elif defined(PLATFORM_POSIX)
    result = close(file->fd) == 0;
#else
    result = fclose(file->fd) == 0;
#endif
    file->fd = INVALID_FILE_HANDLE;
    return result;
}

size_t io_read(file_t file, void *buffer, size_t size) {
    if (!io_valid(file) || !buffer || size <= 0)
        return 0;
#ifdef PLATFORM_WINDOWS
    DWORD bytes_read;
    BOOL result = ReadFile(file.fd, buffer, (DWORD)size, &bytes_read, NULL);
    return result ? bytes_read : 0;
#elif defined(PLATFORM_POSIX)
    return read(file.fd, buffer, size);
#else
    return fread(buffer, size, 1, file.fd);
#endif
}

size_t io_write(file_t file, const void *buffer, size_t size) {
    if (!io_valid(file) || !buffer || size <= 0)
        return 0;
#ifdef PLATFORM_WINDOWS
    DWORD bytes_written;
    BOOL result = WriteFile(file.fd, buffer, (DWORD)size, &bytes_written, NULL);
    return result ? bytes_written : 0;
#elif defined(PLATFORM_POSIX)
    return write(file.fd, buffer, size);
#else
    return fwrite(buffer, size, 1, file.fd);
#endif
}

bool io_seek(file_t file, long offset, enum file_seek_t whence) {
    if (!io_valid(file))
        return false;
#ifdef PLATFORM_WINDOWS
    DWORD move_method;
    switch (whence) {
        case FILE_START:
            move_method = FILE_BEGIN;
            break;
        case FILE_CURSOR:
            move_method = FILE_CURRENT;
            break;
        case FILE_FINISH:
            move_method = FILE_END;
            break;
        default:
            return false;
    }
    return SetFilePointer(file.fd, offset, NULL, move_method) != INVALID_SET_FILE_POINTER;
#elif defined(PLATFORM_POSIX)
    return lseek(file.fd, offset, whence) != -1;
#else
    return fseek(file.fd, offset, whence) == 0;
#endif
}

bool io_advance(file_t file, long offset) {
    return io_seek(file, offset, FILE_CURSOR);
}

size_t io_tell(file_t file) {
    if (!io_valid(file))
        return -1;
#ifdef PLATFORM_WINDOWS
    return (size_t)SetFilePointer(file.fd, 0, NULL, FILE_CURRENT);
    if (pos == INVALID_SET_FILE_POINTER)
        return false;
    if (position)
        *position = (long)pos;
#elif defined(PLATFORM_POSIX)
    return (size_t)lseek(file.fd, 0, FILE_CURSOR);
#else
    return (size_t)fseek(file.fd);
#endif
    return -1;
}

bool io_eof(file_t file) {
    if (!io_valid(file))
        return true;
#ifdef PLATFORM_WINDOWS
    DWORD current_pos = SetFilePointer(file.fd, 0, NULL, FILE_CURRENT);
    if (current_pos == INVALID_SET_FILE_POINTER)
        return true;
    DWORD fsize = GetFileSize(file.fd, NULL);
    if (fsize == INVALID_FILE_SIZE)
        return true;
    return current_pos >= fsize;
#elif defined(PLATFORM_POSIX)
    off_t current_pos = lseek(file.fd, 0, FILE_CURSOR);
    if (current_pos == -1)
        return true;
    off_t end_pos = lseek(file.fd, 0, FILE_FINISH);
    if (end_pos == -1)
        return true;
    lseek(file.fd, current_pos, FILE_START);
    return current_pos >= end_pos;
#else
    return feof(file.fd);
#endif
}

bool io_flush(file_t file) {
    if (!io_valid(file))
        return false;
#ifdef PLATFORM_WINDOWS
    return FlushFileBuffers(file.fd) != 0;
#elif defined(PLATFORM_POSIX)
    return fsync(file.fd) == 0;
#else
    return fflush(file.fd) == 0;
#endif
}

bool io_valid(file_t file) {
#ifdef PLATFORM_UNKNOWN
    return file.fd != NULL;
#else
    return file.fd >= 0;
#endif
}

bool io_read_line(file_t file, char *buffer, size_t buffer_size) {
    if (!io_valid(file) || !buffer || buffer_size <= 0)
        return false;
    size_t pos = 0;
    char ch;
    while (pos < buffer_size - 1) {
        if (io_read(file, &ch, 1) != 1)
            break;
        if (ch == '\n')
            break;
        if (ch != '\r')
            buffer[pos++] = ch;
    }
    buffer[pos] = '\0';
    return pos > 0 || !io_eof(file);
}

bool io_write_string(file_t file, const char *str) {
    if (!io_valid(file) || !str)
        return false;
    size_t len = strlen(str);
    if (!len)
        return true;
    return io_write(file, str, len) == len;
}

bool io_truncate(file_t file, long size) {
    if (!io_valid(file) || size < 0)
        return false;
#ifdef PLATFORM_WINDOWS
    if (SetFilePointer(file.fd, size, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        return false;
    return SetEndOfFile(file.fd) != 0;
#elif defined(PLATFORM_POSIX)    
    return ftruncate(file.fd, size) == 0;
#else
    long fsize, pos;
    if ((pos = io_tell(file)) == -1|| 
        !io_seek(file, 0, FILE_FINISH) || 
        (fsize = io_tell(file)) == -1)
        return false;
    return size > fsize ? true : io_seek(file, pos <= size ? pos : size, FILE_CURSOR);
#endif
}

char* enviroment_variable(const char *var_name) {
    if (!var_name)
        return NULL;
#ifdef PLATFORM_WINDOWS
    DWORD buffer_size = GetEnvironmentVariableA(var_name, NULL, 0);
    if (!buffer_size)
        return NULL;
    char* value = malloc(buffer_size * sizeof(char));
    if (!value)
        return NULL;
    DWORD result = GetEnvironmentVariableA(var_name, value, buffer_size);
    if (result == 0 || result >= buffer_size) {
        free(value);
        return NULL;
    }
    return value;
#elif defined(PLATFORM_POSIX)
    for (int i = 0; environ[i] != NULL; i++) {
        char* env_str = environ[i];
        char* equals_sign = NULL;
        for (int j = 0; env_str[j] != '\0'; j++)
            if (env_str[j] == '=') {
                equals_sign = &env_str[j];
                break;
            }
        if (!equals_sign)
            continue;
        size_t name_len = equals_sign - env_str;
        if (strlen(var_name) == name_len && strncmp(env_str, var_name, name_len)) {
            const char* env_value = equals_sign + 1;
            size_t value_len = strlen(env_value);
            char *value = malloc(value_len + 1);
            if (!value)
                return NULL;
            for (size_t k = 0; k <= value_len; k++)
                value[k] = env_value[k];
            return value;
        }
    }
#endif
    return NULL;
}

bool file_exists(const char *path) {
    if (!path)
        return false;
#ifdef PLATFORM_WINDOWS
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES && !attrs & FILE_ATTRIBUTE_DIRECTORY;
#elif defined(PLATFORM_POSIX)
    struct stat st;
    if (stat(path, &st) == -1)
        return false;
    return S_ISREG(st.st_mode);
#else
    return false;
#endif
}

bool file_delete(const char *path) {
    if (!path || !directory_exists(path))
        return false;
#ifdef PLATFORM_WINDOWS
    return DeleteFileA(path) != 0;
#elif defined(PLATFORM_POSIX)
    return unlink(path) == 0;
#else
    return false;
#endif
}

bool file_rename(const char *old_path, const char *new_path, bool write_over) {
    if (!old_path || !file_exists(old_path) || !new_path || (!write_over && file_exists(new_path)))
        return false;
#ifdef PLATFORM_WINDOWS
    return MoveFileA(old_path, new_path) != 0;
#elif defined(PLATFORM_POSIX)
    if (link(old_path, new_path) == 0)
        return unlink(old_path) == 0;
#endif
    return false;
}

int file_size(const char *path) {
    if (!path)
        return -1;
#ifdef PLATFORM_WINDOWS
    WIN32_FILE_ATTRIBUTE_DATA file_data;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &file_data))
        return -1;
    return ((long long)file_data.nFileSizeHigh << 32) | file_data.nFileSizeLow;
#elif defined(PLATFORM_POSIX)
    struct stat st;
    if (stat(path, &st) == -1)
        return -1;
    return (int)st.st_size;
#else
    return -1;
#endif
}

bool file_copy(const char *src_path, const char *dst_path, bool write_over) {
    if (!src_path || !dst_path || !file_exists(src_path) || (path_exists(dst_path) && !write_over))
        return false;

    static const size_t buffer_size = 4096;
    file_t src_fd = {-1}, dst_fd = {-1};
    ssize_t bytes_read, bytes_written;
    char buffer[buffer_size];
    bool result = false;

    if (!io_open(&src_fd, src_path, FILE_READ))
        goto BAIL;
    if (!io_open(&dst_fd, dst_path, FILE_WRITE))
        goto BAIL;

    while ((bytes_read = io_read(src_fd, buffer, buffer_size)) > 0)
        if ((bytes_written = io_write(dst_fd, buffer, bytes_read)) != bytes_read)
            goto BAIL;
    if (bytes_read == -1)
        goto BAIL;
    result = true;

BAIL:
    if (io_valid(src_fd))
        io_close(&src_fd);
    if (io_valid(dst_fd))
        io_close(&dst_fd);
    return result;
}

const char* file_read(const char *path, size_t *size) {
    char *result = NULL;
    long _size = -1;
    file_t file = io_invalid;
    if (!path || !file_exists(path) || !io_open(&file, path, FILE_READ))
        goto BAIL;
    io_seek(file, 0, FILE_START);
    io_seek(file, 0, FILE_FINISH);
    if ((_size = io_tell(file)) == -1)
        goto BAIL;
    io_seek(file, 0, FILE_START);
    if (!(result = malloc(_size * sizeof(char))))
        goto BAIL;
    if (!io_read(file, result, _size)) {
        free(result);
        goto BAIL;
    }
BAIL:
    if (io_valid(file))
        io_close(&file);
    if (size)
        *size = _size;
    return result;
}

bool directory_exists(const char *path) {
    if (!path)
        return false;
#ifdef PLATFORM_WINDOWS
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES && attr & FILE_ATTRIBUTE_DIRECTORY;
#elif defined(PLATFORM_POSIX)
    struct stat st;
    if (stat(path, &st) == -1)
        return false;
    return S_ISDIR(st.st_mode);
#else
    return false;
#endif
}

bool directory_create(const char *path, bool recursive) {
    if (!path)
        return false;
    if (directory_exists(path))
        return true;
#ifdef PLATFORM_WINDOWS
    return CreateDirectoryA(path, NULL) != 0;
#elif defined(PLATFORM_POSIX)
    if (recursive) {
        const char *resolved = path_resolve(path);
        if (!resolved)
            return false;
        while (true) {
            const char *parent = path_get_parent_directory(resolved);
            if (!parent) {
                free((void*)resolved);
                return false;
            }
            if (directory_exists(resolved)) {
                free((void*)resolved);
                free((void*)parent);
                return true;
            } else {
                if (mkdir(resolved, 0755) != 0) {
                    free((void*)resolved);
                    free((void*)parent);
                    return false;
                }
            }
            free((void*)resolved);
            resolved = parent;
        }
        free((void*)resolved);
        return true;
    } else
        return mkdir(path, 0755) == 0;
#else
    return false;
#endif
}

bool directory_delete(const char *path, bool recursive, bool and_files) {
    if (!path || !directory_exists(path))
        return false;

#ifndef PLATFORM_UNKNOWN
    if (recursive) {
        dir_t d = {.path=path};
        const char *name = NULL;
        bool is_dir = false;
        while ((name = directory_iter(&d, &is_dir)) != NULL) {
            const char *full_path = path_join(path, name);
            if (is_dir) {
                if (!directory_delete(full_path, true, and_files)) {
                    directory_iter_end(&d);
                    free((void*)full_path);
                    return false;
                }
            } else {
                if (!file_delete(full_path)) {
                    directory_iter_end(&d);
                    free((void*)full_path);
                    return false;
                }
            }
            free((void *)full_path);
        }
    } else {
        if (and_files) {
            dir_t d = {.path=path};
            const char *name = NULL;
            bool is_dir = false;
            const char **files = malloc(0);
            size_t files_size = 0;
            if (!files)
                return false;
            while ((name = directory_iter(&d, &is_dir)) != NULL) {
                if (is_dir) {
                    for (int i = 0; i < files_size; i++)
                        free((void *)files[i]);
                    free((void*)files);
                    directory_iter_end(&d);
                    return false;
                }
                const char *full_path = path_join(path, name);
                if (!full_path) {
                    directory_iter_end(&d);
                    return false;
                }
                if (!(files = realloc(files, ++files_size * sizeof(char *)))) {
                    free((void *)full_path);
                    directory_iter_end(&d);
                    return false;
                }
                files[files_size - 1] = full_path;
            }
            bool success = true;
            if (files_size > 0) {
                for (size_t i = 0; i < files_size; i++) {
                    if (!file_delete(files[i])) {
                        success = false;
                        break;
                    }
                }
            }
            for (size_t i = 0; i < files_size; i++)
                free((void *)files[i]);
            free((void *)files);
            if (!success)
                return false;
        }
    }
#endif

#ifdef PLATFORM_WINDOWS
    return RemoveDirectoryA(path) != 0;
#elif defined(PLATFORM_POSIX)
    return rmdir(path) == 0;
#else
    return false;
#endif
}

bool directory_rename(const char *old_path, const char *new_path, bool write_over) {
    return directory_copy(old_path, new_path, write_over, true);
}

bool directory_copy(const char *src_path, const char *dst_path, bool write_over, bool delete_src) {
    if (!src_path || !directory_exists(src_path) || !dst_path)
        return false;

    bool result = false;
    const char *parent = path_get_parent_directory(dst_path);
    const char *dir_name = NULL, *dst = NULL;
    if (!parent || !directory_exists(parent))
        goto BAIL;
    if (!(dir_name = path_get_directory_name(src_path)))
        goto BAIL;
    if (!(dst = path_join(dst_path, dir_name)) || !directory_create(dst, false))
        goto BAIL;

    dir_t d = {.path=src_path};
    const char *name = NULL;
    bool is_dir = false;
    while ((name = directory_iter(&d, &is_dir)) != NULL) {
        const char *src_full = path_join(src_path, name);
        const char *dst_full = path_join(dst_path, name);
        bool die = false;
        if (is_dir)
            if (directory_create(dst_full, false)) {
                if (!directory_copy(src_full, dst_full, write_over, delete_src))
                    die = true;
            } else
                die = true;
        else
            if (file_copy(src_full, dst_full, write_over))
                if (delete_src)
                    file_delete(src_full);
        free((void *)src_full);
        free((void *)dst_full);
        if (die) {
            directory_iter_end(&d);
            goto BAIL;
        }
    }

    if (delete_src)
        directory_delete(src_path, true, true);
    result = true;
BAIL:
    if (parent)
        free((void*)parent);
    if (dir_name)
        free((void*)dir_name);
    if (dst)
        free((void*)dst);
    return result;
}

int directory_size(const char *path) {
    if (!path || !directory_exists(path))
        return -1;
    dir_t d = {.path=path};
    const char *name = NULL;
    bool is_dir = false;
    int total = 0;
    while ((name = directory_iter(&d, &is_dir)) != NULL) {
        const char *full_path = path_join(path, name);
        if (!full_path) {
            directory_iter_end(&d);
            return -1;
        }
        total += is_dir ? directory_size(full_path) : file_size(full_path);
        free((void *)full_path);
    }
    return total;
}

int directory_item_count(const char *path, bool recursive) {
    if (!path || !directory_exists(path))
        return -1;
    dir_t d = {.path=path};
    const char *name = NULL;
    bool is_dir = false;
    int total = 0;
    while ((name = directory_iter(&d, &is_dir)) != NULL) {
        total++;
        if (is_dir && recursive) {
            const char *full_path = path_join(path, name);
            if (!full_path) {
                directory_iter_end(&d);
                return -1;
            }
            total += directory_file_count(full_path, true);
            free((void *)full_path);
        }
    }
    return total;
}

int directory_file_count(const char *path, bool recursive) {
    if (!path || !directory_exists(path))
        return -1;
    dir_t d = {.path=path};
    const char *name = NULL;
    bool is_dir = false;
    int total = 0;
    while ((name = directory_iter(&d, &is_dir)) != NULL) {
        if (is_dir) {
            if (recursive) {
                const char *full_path = path_join(path, name);
                if (!full_path) {
                    directory_iter_end(&d);
                    return -1;
                }
                total += directory_file_count(full_path, true);
                free((void*)full_path);
            }
        } else
            total++;
    }
    return total;
}

const char** directory_glob(const char *pattern, bool recursive, int *count) {
    return NULL; // TODO
}

const char* directory_iter(dir_t *dir, bool *is_dir) {
#ifdef PLATFORM_UNKNOWN
    return NULL;
#else
    if (!dir || !dir->path)
        return NULL;
#ifdef PLATFORM_WINDOWS
NEXT:
    if (dir->hFind <= 0) {
        if (!directory_exists(dir->path) || 
            (hFind = FindFirstFile(dir->path, &dir->findData)) == INVALID_HANDLE_VALUE)
            return NULL;
    } else {
        if (!FindNextFile(hFind, &findData)) {
            FindClose(dir->hFind);
            return NULL;
        }
    }
    
    if (!strcmp(dir->findData.cFileName, ".") ||
        !strcmp(dir->findData.cFileName, ".."))
        goto NEXT;
    if (dir->findData.dwFileAttributes == INVALID_FILE_ATTRIBUTES)
        goto NEXT;
    if (is_dir)
        *is_dir = (dir->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    return dir->findData.cFileName;
#else
    if (!dir->dir)
        if (!directory_exists(dir->path) || !(dir->dir = opendir(dir->path)))
            return NULL;

    struct dirent* entry = NULL;
NEXT:
    if (!(entry = readdir(dir->dir))) {
        closedir(dir->dir);
        return NULL;
    }

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        goto NEXT;
    const char *full_path = path_join(dir->path, entry->d_name);
    struct stat _stat;
    if (stat(full_path, &_stat) == -1) {
        free((void*)full_path);
        goto NEXT;
    }
    free((void*)full_path);
    
    if (is_dir)
        *is_dir = S_ISDIR(_stat.st_mode);
    return entry->d_name;
#endif
#endif
}

void directory_iter_end(dir_t *dir) {
    if (!dir)
        return;
#ifdef PLATFORM_WINDOWS
    FindClose(dir->hFind);
#elif defined(PLATFORM_POSIX)
    closedir(dir->dir);
#endif
}

bool path_glob(const char *pattern, bool recursive, glob_callback callback, void *userdata) {
    return -1;
}

#define _WALK(FN, PATH, RECURSIVE, CALLBACK, USERDATA)                     \
    if (!(PATH) || !directory_exists(PATH))                                \
        return false;                                                      \
    dir_t d = {.path = (PATH)};                                            \
    const char *name = NULL;                                               \
    bool is_dir = false;                                                   \
    while ((name = directory_iter(&d, &is_dir)) != NULL)                   \
    {                                                                      \
        const char *full_path = path_join((PATH), name);                   \
        if (is_dir)                                                        \
        {                                                                  \
            if (RECURSIVE)                                                 \
                if (FN(full_path, RECURSIVE, CALLBACK, USERDATA) == false) \
                    goto BAIL;                                             \
        }                                                                  \
        else if (!CALLBACK((PATH), name, USERDATA))                        \
            goto BAIL;                                                     \
        free((void *)full_path);                                           \
        continue;                                                          \
    BAIL:                                                                  \
        directory_iter_end(&d);                                            \
        free((void *)full_path);                                           \
        return false;                                                      \
    }                                                                      \
    return true;

bool path_walk(const char *path, bool recursive, walk_callback callback, void *userdata) {
    _WALK(path_walk, path, recursive, callback, userdata);
}

#ifdef PLATFORM_HAS_BLOCKS
bool path_glob_block(const char *pattern, bool recursive, glob_block callback, void *userdata) {
    return -1;
}

bool path_walk_block(const char *path, bool recursive, walk_block callback, void *userdata) {
    _WALK(path_walk_block, path, recursive, callback, userdata);
}
#endif // PLATFORM_HAS_BLOCKS

bool path_exists(const char *path) {
    if (!path)
        return false;
#ifdef PLATFORM_WINDOWS
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES;
#elif defined(PLATFORM_POSIX)
    struct stat st;
    if (stat(path, &st) == -1)
        return false;
    return S_ISDIR(st.st_mode) || S_ISREG(st.st_mode);
#else
    return false;
#endif
}

const char* path_get_working_directory(void) {
    static char buffer[MAX_PATH];
    memset(buffer, 0, MAX_PATH * sizeof(char));
#ifdef PLATFORM_WINDOWS
    DWORD result = GetCurrentDirectoryA((DWORD)MAX_PATH, buffer);
    return result > 0 && result < buffer_size;
#elif defined(PLATFORM_POSIX)
    return getcwd(buffer, MAX_PATH);
#endif
}

bool path_set_working_directory(const char *path) {
    if (!path || !directory_exists(path))
        return false;
#ifdef PLATFORM_WINDOWS
    return SetCurrentDirectoryA(path) != 0;
#elif defined(PLATFORM_POSIX)
    return chdir(path) == 0;
#endif
}

const char* path_get_root_dir(void) {
#ifdef PLATFORM_WINDOWS
    static char root[4]  = {'\0'};
    if (root[0] == '\0') {
        char tmp[MAX_PATH];
        GetWindowsDirectory(tmp, MAX_PATH);
        memcpy(root, tmp, 4 * sizeof(char));
    }
    return root;
#elif defined(PLATFORM_POSIX)
    return "/";
#else
    return NULL;
#endif
}

#ifdef PLATFORM_WINDOWS
static const char* __get_path(const GUID* folderid) {
    LPWSTR path;
    HRESULT result = SHGetKnownFolderPath(folderid, 0, NULL, &path);
    if (result != S_OK)
        return NULL;
    int size = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
    if (size == 0) {
        CoTaskMemFree(path);
        return NULL;
    }
    static const char root[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, path, -1, root, size, NULL, NULL);
    CoTaskMemFree(path);
    return root;
}
#endif

#ifdef PLATFORM_MACOS
#ifndef __OBJC__
typedef enum {
    NSApplicationDirectory  = 1,
    NSDocumentDirectory     = 9,
    NSDesktopDirectory      = 12,
    NSDownloadsDirectory    = 15,
    NSMoviesDirectory       = 17,
    NSMusicDirectory        = 18,
    NSPicturesDirectory     = 19,
} NSSearchPathDirectory;

typedef enum {
    NSUserDomainMask        = 1UL << 0,
    NSLocalDomainMask       = 1UL << 1,
    NSNetworkDomainMask     = 1UL << 2,
    NSSystemDomainMask      = 1UL << 3,
    NSAllDomainsMask        = 0x0FFFFFFF
} NSSearchPathDomainMask;
#endif

static const char* __get_path(NSSearchPathDirectory directory) {
    id file_manager = ObjC(id)(class(NSFileManager), sel(defaultManager));
    id urls = ObjC(id, NSUInteger, NSUInteger)(file_manager, sel(URLForDirectory:inDomains:), directory, 1);
    id url = ObjC(id)(urls, sel(firstObject));
    const char *path = (const char*)ObjC(id)(ObjC(id)(url, sel(absoluteString)), sel(UTF8String));
    long path_length = strlen(path);
    static char _path[MAX_PATH];
    memcpy(_path, path, path_length * sizeof(char));
    if (path_length < MAX_PATH)
        _path[path_length] = '\0';
    return _path;
}
#endif

#ifdef PLATFORM_NIX
static const char* __get_path_ptr(const char *name) {
    const char *home = path_get_home_dir();
    const char *join = path_join(home, name);
    const char *_name = strdup(name);
    size_t name_len = strlen(name);
    file_t ud = io_invalid;

    if (directory_exists(join))
        goto BAIL;

    for (char *p = _name; *p; p++)
        if (*p >= 'a' && *p <= 'z')
            *p -= 32;
    const char *user_dirs = path_join(home, ".config/user-dirs.dirs");
    if (!io_open(&ud, user_dirs, FILE_READ))
        goto BAIL;

    char line[1024];
    while (!io_eof(ud)) {
        memset(line, 0, 1024 * sizeof(char));
        if (!io_read_line(ud, line, 1024))
            break;
        if (!strncmp("XDG_", line, 3) &&
            !strncmp(_name, line + 4, name_len)) {
            free((void*)join);
            free((void*)_name);
            char *l = line;
            for (; *l && *l != '\n'; l++);
            if (*l == '\n')
                *l = '\0';

            char *off = line + (7 + name_len);
            bool is_home = false;
            if (!strncmp("$HOME", off, 5)) {
                is_home = true;
                off += 5;
            }
            if (off[0] == PATH_SEPERATOR)
                off++;
            char *p = off;
            for (; *p && *p != '"'; p++);
            size_t diff = p - off;
            memcpy(line, off, diff * sizeof(char));
            memset(line + diff, 0, (1024 - diff) * sizeof(char));
            if (is_home)
                return path_join(home, line);
            else
                return strdup(line);
        }
    }
    
BAIL:
    if (io_valid(ud))
        io_close(&ud);
    if (_name)
        free((void*)_name);
    return join;
}

static const char* __get_path(const char *name) {
    static char path[MAX_PATH];
    memset(path, 0, MAX_PATH * sizeof(char));
    const char *tmp = __get_path_ptr(name);
    if (tmp) {
        memcpy(path, tmp, strlen(tmp) * sizeof(char));
        free((void*)tmp);
    }
    return path;
}
#endif

const char* path_get_home_dir(void) {
#ifdef PLATFORM_UNKNOWN
    return NULL;
#else
    static char home[MAX_PATH] = {'\0'};
    if (home[0] == '\0') {
        const char *_home = NULL;
        size_t _size = 0;
#ifdef PLATFORM_WINDOWS
        _home = __get_path(&FOLDERID_Profile);
        _size = strlen(_home);
#elif defined(PLATFORM_MACOS)
        id file_manager = ObjC(id)(class(NSFileManager), sel(defaultManager));
        id url = ObjC(id)(file_manager, sel(homeDirectory));
        _home = (const char*)ObjC(id)(ObjC(id)(url, sel(absoluteString)), sel(UTF8String));
        _size = strlen(_home);
#elif defined(PLATFORM_NIX)
        if (!(_home = enviroment_variable("HOME")))
            return NULL;
        _size = strlen(_home);
#endif
        if (_size > MAX_PATH)
            _size = MAX_PATH;
        memcpy(home, _home, _size * sizeof(char));
        memset(home + _size, 0, MAX_PATH - _size * sizeof(char));
#ifdef PLATFORM_NIX
        free((void*)_home);
#endif
    }
    return home;
#endif
}

const char* path_get_documents_dir(void) {
#ifdef PLATFORM_WINDOWS
    return __get_path(&FOLDERID_Documents);
#elif defined(PLATFORM_MACOS)
    return __get_path(NSDocumentDirectory);
#elif defined(PLATFORM_NIX)
    return __get_path("Documents");
#else
    return NULL;
#endif
}

const char* path_get_downloads_dir(void) {
#ifdef PLATFORM_WINDOWS
    return __get_path(&FOLDERID_Downloads);
#elif defined(PLATFORM_MACOS)
    return __get_path(NSDownloadsDirectory);
#elif defined(PLATFORM_NIX)
    return __get_path("Downloads");
#else
    return NULL;
#endif
}

const char* path_get_video_dir(void) {
#ifdef PLATFORM_WINDOWS
    return __get_path(&FOLDERID_Videos);
#elif defined(PLATFORM_MACOS)
    return __get_path(NSMoviesDirectory);
#elif defined(PLATFORM_NIX)
    return __get_path("Videos");
#else
    return NULL;
#endif
}

const char* path_get_music_dir(void) {
#ifdef PLATFORM_WINDOWS
    return __get_path(&FOLDERID_Music);
#elif defined(PLATFORM_MACOS)
    return __get_path(NSMusicDirectory);
#elif defined(PLATFORM_NIX)
    return __get_path("Music");
#else
    return NULL;
#endif
}

const char* path_get_picture_dir(void) {
#ifdef PLATFORM_WINDOWS
    return __get_path(&FOLDERID_Pictures);
#elif defined(PLATFORM_MACOS)
    return __get_path(NSPicturesDirectory);
#elif defined(PLATFORM_NIX)
    return __get_path("Pictures");
#else
    return NULL;
#endif
}

const char* path_get_application_dir(void) {
#ifdef PLATFORM_WINDOWS
    return __get_path(&FOLDERID_ProgramFiles);
#elif defined(PLATFORM_MACOS)
    return __get_path(NSApplicationDirectory);
#else
    return NULL;
#endif
}

const char* path_get_desktop_dir(void) {
#ifdef PLATFORM_WINDOWS
    return __get_path(&FOLDERID_Desktop);
#elif defined(PLATFORM_MACOS)
    return __get_path(NSDesktopDirectory);
#elif defined(PLATFORM_NIX)
    return __get_path("Desktop");
#else
    return NULL;
#endif
}

const char* path_get_file_extension(const char *path) {
    if (!path)
        return NULL;
    char *dot = strrchr(path, '.');
    return dot ? dot + 1 : NULL;
}

const char* path_get_file_name(const char *path) {
    if (!path)
        return NULL;
    const char *last_slash = strrchr(path, PATH_SEPERATOR);
    return last_slash ? last_slash + 1 : path;
}

const char* path_get_file_name_no_extension(const char *path) {
    if (!path)
        return NULL;
    const char *file_name = path_get_file_name(path);
    if (!file_name)
        return NULL;
    size_t length = strlen(file_name);
    char *dot = strrchr(file_name, '.');
    if (dot) {
        length = dot - file_name;
        if (length == 0)
            return NULL;
    }
    char *result = malloc(length + 1);
    if (!result)
        return NULL;
    strncpy(result, file_name, length);
    result[length] = '\0';
    return result;
}

const char* path_without_file_name(const char *path) {
    if (!path)
        return NULL;
    const char *last_slash = strrchr(path, PATH_SEPERATOR);
    if (!last_slash)
        return NULL;
    size_t length = last_slash - path;
    char *result = malloc(length + 1);
    if (!result)
        return NULL;
    strncpy(result, path, length);
    result[length] = '\0';
    return result;
}

const char* path_get_directory_name(const char *path) {
    const char *stem = path_without_file_name(path);
    if (!stem)
        return NULL;
    const char *last_slash = strrchr(stem, PATH_SEPERATOR);
    return last_slash ? last_slash + 1 : stem;
}

const char* path_get_parent_directory(const char *path) {
    if (!path)
        return NULL;
    const char *last_slash = strrchr(path, PATH_SEPERATOR);
    if (!last_slash || last_slash == path)
        return path_get_root_dir();
    size_t length = last_slash - path;
    char *result = malloc(length + 1);
    if (!result)
        return NULL;
    strncpy(result, path, length);
    result[length] = '\0';
    return NULL;
}

const char* path_resolve(const char *path) {
    if (!path)
        return NULL;
    if (*path == '~') {
        const char *tmp = path_join(path_get_home_dir(), path + 1);
        const char *full = path_resolve(tmp);
        free((void*)tmp);
        return full;
    }
    if (!strncmp(path_get_root_dir(), path, strlen(path_get_root_dir())))
        return strdup(path);
    const char *full = path_join(path_get_working_directory(), path);
    size_t parts_count = 0;
    const char **parts = path_split(full, &parts_count);
    free((void*)full);
    if (!parts || !parts_count)
        return NULL;
    const char *p = *parts;
    char *result = NULL;
    int index = 0;
    while (p) {
        size_t len = strlen(p);
        if (len == 1 && *p == '.') {
            const char **new_parts = malloc(--parts_count * sizeof(char*));
            if (!new_parts)
                goto BAIL;
            for (int i = 0, j = 0; i < parts_count + 1; i++)
                if (i != index)
                    new_parts[j++] = parts[i];
            free((void*)parts);
            parts = new_parts;
            parts_count--;
            if (!(p = parts[index]))
                goto BAIL;
        } else if (len == 2 && !strncmp(p, "..", 2)) {
            if (index == 0)
                goto BAIL;
            const char **new_parts = malloc(parts_count - 2 * sizeof(char*));
            if (!new_parts)
                goto BAIL;
            for (int i = 0, j = 0; i < parts_count; i++)
                if (i != index && i != index - 1)
                    new_parts[j++] = parts[i];
            free((void*)parts);
            parts = new_parts;
            parts_count -= 2;
            index--;
            if (index < 0)
                index = 0;
            if (parts_count == 0)
                goto BAIL;
            if (!(p = parts[index]))
                goto BAIL;
        } else {
            if (!(p = *(++parts)))
                goto BAIL;
            index++;
        }
    }
    char buffer[MAX_PATH];
    size_t root_len = strlen(path_get_root_dir());
    size_t length = root_len;
    memcpy(buffer, path_get_root_dir(), root_len * sizeof(char));
    for (int i = 0; i < parts_count; i++) {
        if (i > 0 && i != parts_count - 1)
            buffer[length++] = PATH_SEPERATOR;
        size_t part_len = strlen(parts[i]);
        if (part_len + length + 1 >= MAX_PATH)
            goto BAIL;
        memcpy(buffer + length, parts[i], part_len * sizeof(char));
        length += part_len;
    }
    if (!(result = malloc(length + 1 * sizeof(char))))
        goto BAIL;
    memcpy(result, buffer, length * sizeof(char));
    result[length] = '\0';
BAIL:
    if (parts) {
        for (int j = 0; j < parts_count; j++)
            free((void *)parts[j]);
        free((void *)parts);
    }
    return (const char*)result;
}

const char* path_join(const char *a, const char *b) {
    if (!a || !b)
        return NULL;
    size_t a_len = strlen(a);
    size_t b_len = strlen(b);
    if (!a_len || !b_len)
        return NULL;
    size_t total_len = a_len + b_len + 2;
    char *result = malloc(total_len);
    if (!result)
        return NULL;
    memcpy(result, a, a_len * sizeof(char));
    memcpy(result + a_len, PATH_SEPERATOR_STR, sizeof(char));
    memcpy(result + a_len + 1, b, b_len * sizeof(char));
    result[total_len] = '\0';
    return result;
}

const char* path_join_va(int n, ...) {
    if (n <= 0)
        return NULL;
    va_list args;
    va_start(args, n);
    size_t total_length = 0;
    for (int i = 0; i < n; i++) {
        const char *part = va_arg(args, const char*);
        if (part) {
            total_length += strlen(part);
            if (i < n - 1)
                total_length += 1;
        }
    }
    va_end(args);
    if (total_length == 0)
        return NULL;
    char *result = malloc(total_length + 1);
    if (!result)
        return NULL;
    
    va_start(args, n);
    size_t pos = 0;
    for (int i = 0; i < n; i++) {
        const char *part = va_arg(args, const char*);
        if (part) {
            size_t part_length = strlen(part);
            memcpy(result + pos, part, part_length);
            pos += part_length;
            if (i < n - 1)
                result[pos++] = PATH_SEPERATOR;
        }
    }
    va_end(args);
    result[pos] = '\0';
    return result;
}

const char** path_split(const char *path, size_t *count) {
    const char **parts = NULL;
    int parts_count = 0;
    if (!path)
        goto BAIL;
    const char *start = path;
    for (const char *p = path; *p; p++)
        if (*p == PATH_SEPERATOR) {
            if (p > start)
                parts_count++;
            start = p + 1;
        }
    if (start < path + strlen(path))
        parts_count++;
    if (parts_count == 0)
        goto BAIL;
    
    if (!(parts = malloc(parts_count * sizeof(char*))))
        goto BAIL;
    start = path;
    int index = 0;
    for (const char *p = path; *p; p++)
        if (*p == PATH_SEPERATOR) {
            if (p > start) {
                size_t length = p - start;
                parts[index] = malloc(length + 1);
                if (!parts[index])
                    goto DEAD;
                strncpy((char*)parts[index], start, length);
                ((char*)parts[index])[length] = '\0';
                index++;
            }
            start = p + 1;
        }
    if (start < path + strlen(path)) {
        size_t length = path + strlen(path) - start;
        parts[index] = malloc(length + 1);
        if (!parts[index])
            goto DEAD;
        strncpy((char*)parts[index], start, length);
        ((char*)parts[index])[length] = '\0';
        index++;
    }
    goto BAIL;
DEAD:
    if (parts) {
        for (int i = 0; i < index; i++)
            if (parts[i])
                free((void*)parts[i]);
        free(parts);
    }
    parts = NULL;
    parts_count = -1;
BAIL:
    if (count)
        *count = parts_count;
    return parts;
}
#endif
