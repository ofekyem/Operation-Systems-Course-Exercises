
#include "copytree.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#define PATH_MAX 4096

void copy_file(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    int src_fd, dest_fd;
    ssize_t bytes_read;
    char buffer[4096];
    struct stat src_stat;
    mode_t mode;

    if (copy_symlinks) {
        if (lstat(src, &src_stat) == -1) {
            perror("error, lstat failed");
            exit(EXIT_FAILURE);
        }
        if (S_ISLNK(src_stat.st_mode)) {
            char link_target[PATH_MAX];
            ssize_t length = readlink(src, link_target, sizeof(link_target) - 1);
            if (length == -1) {
                perror("error with readlink");
                exit(EXIT_FAILURE);
            }
            link_target[length] = '\0';
            if (symlink(link_target, dest) == -1) {
                perror("error with symlink");
                exit(EXIT_FAILURE);
            }
            return;
        }
    }

    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("error with open");
        exit(EXIT_FAILURE);
    }

    if (fstat(src_fd, &src_stat) == -1) {
        perror("error with fstat");
        close(src_fd);
        exit(EXIT_FAILURE);
    }

    mode = copy_permissions ? src_stat.st_mode : (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (dest_fd == -1) {
        perror("error with open");
        close(src_fd);
        exit(EXIT_FAILURE);
    }

    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        if (write(dest_fd, buffer, bytes_read) != bytes_read) {
            perror("error with write");
            close(src_fd);
            close(dest_fd);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_read == -1) {
        perror("error with read");
        close(src_fd);
        close(dest_fd);
        exit(EXIT_FAILURE);
    }

    if (close(src_fd) == -1) {
        perror("error with close");
        close(dest_fd);
        exit(EXIT_FAILURE);
    }

    if (close(dest_fd) == -1) {
        perror("error with close");
        exit(EXIT_FAILURE);
    }
}

void create_directory(const char *path, mode_t mode) {
    char tmp[PATH_MAX];
    char *p_str = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (p_str = tmp + 1; *p_str; p_str++) {
        if (*p_str == '/') {
            *p_str = '\0';
            if (mkdir(tmp, mode) == -1 && errno != EEXIST) {
                perror("error with mkdir");
                exit(EXIT_FAILURE);
            }
            *p_str = '/';
        }
    }
    if (mkdir(tmp, mode) == -1 && errno != EEXIST) {
        perror("error with mkdir");
        exit(EXIT_FAILURE);
    }
}

void copy_directory(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    DIR *dir;
    struct dirent *entry;
    struct stat src_stat;
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];

    dir = opendir(src);
    if (dir == NULL) {
        perror("error with opendir");
        exit(EXIT_FAILURE);
    }

    create_directory(dest, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    errno = 0;  
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);

        if (lstat(src_path, &src_stat) == -1) {
            perror("error with lstat");
            closedir(dir); 
            exit(EXIT_FAILURE);
        }

        if (S_ISDIR(src_stat.st_mode)) {
            copy_directory(src_path, dest_path, copy_symlinks, copy_permissions);
        } else {
            copy_file(src_path, dest_path, copy_symlinks, copy_permissions);
        }
    }

    if (errno != 0) {
        perror("error with readdir");
        closedir(dir); 
        exit(EXIT_FAILURE);
    }

    if (closedir(dir) == -1) {
        perror("error with closedir");
        exit(EXIT_FAILURE);
    }
}
