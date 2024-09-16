
#include "buffered_open.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

buffered_file_t *buffered_open(const char *pathname, int flags, ...) {
    buffered_file_t *bf = malloc(sizeof(buffered_file_t));
    if (!bf) {
        perror("error in dynamic allocation");
        return NULL;
    }

    if (flags & O_PREAPPEND) {
        bf->preappend = 1;
        flags &= ~O_PREAPPEND;
    } else {
        bf->preappend = 0;
    }

    bf->flags = flags;
    bf->write_buffer_size = BUFFER_SIZE;
    bf->write_buffer = malloc(bf->write_buffer_size);
    if (!bf->write_buffer) {
        perror("error in dynamic allocation");
        free(bf);
        return NULL;
    }

    bf->write_buffer_pos = 0;

    bf->read_buffer_size = BUFFER_SIZE;
    bf->read_buffer = malloc(bf->read_buffer_size);
    if (!bf->read_buffer) {
        perror("error in dynamic allocation");
        free(bf->write_buffer);
        free(bf);
        return NULL;
    }

    bf->read_buffer_pos = 0;

    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode_t mode = va_arg(args, mode_t);
        va_end(args);
        bf->fd = open(pathname, flags, mode);
    } else {
        bf->fd = open(pathname, flags);
    }

    if (bf->fd == -1) {
        perror("error with open");
        free(bf->read_buffer);
        free(bf->write_buffer);
        free(bf);
        return NULL;
    }

    if (flags & O_TRUNC) {
        if (ftruncate(bf->fd, 0) == -1) {
            perror("error with ftruncate");
            close(bf->fd);
            free(bf->read_buffer);
            free(bf->write_buffer);
            free(bf);
            return NULL;
        }
    }

    return bf;
}

ssize_t buffered_write(buffered_file_t *bf, const void *buf, size_t count) {
    size_t remain_space = bf->write_buffer_size - bf->write_buffer_pos;
    size_t total = 0;

    while (count > 0) {
        if (remain_space == 0) { 
            if (buffered_flush(bf) == -1) {
                return -1; 
            }
            remain_space = bf->write_buffer_size; 
        }

        size_t bytes_to_write = (count < remain_space) ? count : remain_space;
        memcpy(bf->write_buffer + bf->write_buffer_pos, buf, bytes_to_write);
        bf->write_buffer_pos += bytes_to_write;
        buf = (const char *)buf + bytes_to_write; 
        count -= bytes_to_write; 
        remain_space -= bytes_to_write;
        total += bytes_to_write;
    }
    if (remain_space == 0) {
        if (buffered_flush(bf) == -1) {
            return -1; 
        }
    }
    return total;
}

ssize_t buffered_read(buffered_file_t *bf, void *buf, size_t count) {
    if (bf->write_buffer_pos > 0) {
        if (buffered_flush(bf) == -1) {
            return -1; 
        }
    }

    if (count > bf->read_buffer_size - bf->read_buffer_pos) {
        size_t remain_space = count;
        size_t total = 0;
        char *buffer = buf;

        while (remain_space > 0) {
            size_t chunk_size = (remain_space< bf->read_buffer_size - bf->read_buffer_pos) ? remain_space : bf->read_buffer_size - bf->read_buffer_pos;
            ssize_t read_bytes = read(bf->fd, buffer, chunk_size);
            if (read_bytes == -1) {
                perror("error in read");
                return -1;
            } else if (read_bytes == 0) { // End of file
                break;
            }
            size_t buffer_space = bf->read_buffer_size - bf->read_buffer_pos;
            size_t bytes_to_copy = (read_bytes < buffer_space) ? read_bytes : buffer_space;
            memcpy(bf->read_buffer + bf->read_buffer_pos, buffer, bytes_to_copy);
            bf->read_buffer_pos += bytes_to_copy;
            if (bf->read_buffer_pos >= bf->read_buffer_size) {
                bf->read_buffer_pos = 0;
            }
            buffer += read_bytes;
            total += read_bytes;
            remain_space -= read_bytes;
        }

        if (total < count) {
            ((char*)buf)[total] = '\0';
        } else {
            ((char*)buf)[count] = '\0';
        }
        return total;

    } else {
        ssize_t read_bytes = read(bf->fd, buf, count);
        if (read_bytes == -1) {
            perror("error in read");
            return -1;
        }
        memcpy(bf->read_buffer + bf->read_buffer_pos, buf, read_bytes);
        bf->read_buffer_pos += read_bytes;
        if (bf->read_buffer_pos >= bf->read_buffer_size) {
                bf->read_buffer_pos = 0;
        }
        if (read_bytes < count) {
            ((char*)buf)[read_bytes] = '\0';
        } else {
            ((char*)buf)[count] = '\0';
        }
        return read_bytes;
    }
}

int buffered_flush(buffered_file_t *bf) {
    if (bf->write_buffer_pos > 0) {
        if (bf->preappend == 1) {
            off_t current_offset = lseek(bf->fd, 0, SEEK_END);
            if (current_offset == -1) {
                perror("error in lseek end");
                return -1; 
            }

            char *temp_bf = (char *)malloc(current_offset);
            if (!temp_bf) {
                perror("error in malloc temp buffer");
                return -1; 
            }
            
            if (lseek(bf->fd, 0, SEEK_SET) == -1) {
                free(temp_bf);
                perror("error in lseek set");
                return -1; 
            }

            ssize_t read_bytes = read(bf->fd, temp_bf, current_offset);
            if (read_bytes == -1) {
                free(temp_bf);
                perror("error in read");
                return -1; 
            }

            if (lseek(bf->fd, 0, SEEK_SET) == -1) {
                free(temp_bf);
                perror("error in lseek set");
                return -1; 
            }

            ssize_t written = write(bf->fd, bf->write_buffer, bf->write_buffer_pos);
            if (written == -1) {
                free(temp_bf);
                perror("error with write buffer");
                return -1; 
            }

            off_t offset = lseek(bf->fd, 0, SEEK_CUR);

            ssize_t write2 = write(bf->fd, temp_bf, read_bytes);
            if (write2 == -1) {
                free(temp_bf);
                perror("error with write temp buffer");
                return -1; 
            }

            if (lseek(bf->fd, offset, SEEK_SET) == -1) {
                perror("error in lseek set");
                return -1;
            }

            free(temp_bf);
            bf->preappend = 2;
        } else if (bf->preappend == 2) {
            off_t saved_offset = lseek(bf->fd, 0, SEEK_CUR);

            off_t current_offset = lseek(bf->fd, 0, SEEK_END);
            if (current_offset == -1) {
                perror("error in lseek end");
                return -1; 
            }

            off_t remaining_size = current_offset - saved_offset;
            char *temp_bf = (char *)malloc(remaining_size);
            if (!temp_bf) {
                perror("error with malloc temp buffer");
                return -1; 
            }

            if (lseek(bf->fd, saved_offset, SEEK_SET) == -1) {
                free(temp_bf);
                perror("error in lseek set");
                return -1; 
            }

            ssize_t read_bytes = read(bf->fd, temp_bf, remaining_size);
            if (read_bytes == -1) {
                free(temp_bf);
                perror("error in read");
                return -1; 
            }

            if (lseek(bf->fd, saved_offset, SEEK_SET) == -1) {
                free(temp_bf);
                perror("error in lseek set");
                return -1; 
            }

            ssize_t written = write(bf->fd, bf->write_buffer, bf->write_buffer_pos);
            if (written == -1) {
                free(temp_bf);
                perror("error with write buffer");
                return -1; 
            }

            ssize_t write2 = write(bf->fd, temp_bf, read_bytes);
            if (write2 == -1) {
                free(temp_bf);
                perror("error with write temp buffer");
                return -1; 
            }

            free(temp_bf);

            if (lseek(bf->fd, saved_offset + written, SEEK_SET) == -1) {
                perror("error in lseek set");
                return -1;
            }
        } else if (bf->flags & O_APPEND) {
            if (lseek(bf->fd, 0, SEEK_END) == -1) {
                perror("error in lseek end");
                return -1;
            }

            ssize_t written = write(bf->fd, bf->write_buffer, bf->write_buffer_pos);
            if (written == -1) {
                perror("error with write buffer");
                return -1; 
            }
        } else {    
            ssize_t written = write(bf->fd, bf->write_buffer, bf->write_buffer_pos);
            if (written == -1) {
                perror("error in write buffer");
                return -1; 
            }
        }
        bf->write_buffer_pos = 0; 
    }
    return 1;
}

int buffered_close(buffered_file_t *bf) {
    if (bf->flags & (O_WRONLY | O_RDWR)) { 
        if (buffered_flush(bf) == -1) {
            return -1;
        }
    }
    int ret = close(bf->fd);
    if (ret == -1) {
        perror("error with close");
    }
    free(bf->read_buffer);
    free(bf->write_buffer);
    free(bf);
    return ret;
}

