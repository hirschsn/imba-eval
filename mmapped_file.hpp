// See LICENSE for license details.

#ifndef MMAPPED_FILE_INCLUDED_
#define MMAPPED_FILE_INCLUDED_

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

inline void eperror(const char *s)
{
    std::perror(s);
    std::exit(1);
}

template <typename T>
struct MFile {
    typedef T value_type;

    MFile(const char *fn, off_t offset = 0): offset(offset), p(nullptr) {
        if ((fd = open(fn, O_RDONLY)) == -1)
            eperror("open");

        struct stat statbuf;
        if (fstat(fd, &statbuf) == -1)
            eperror("fstat");
        bytes = statbuf.st_size;

        // File size and offset must match
        if (bytes < offset) {
            close(fd);
            throw std::runtime_error("Offset > file length.");
        }

        // We do not use "offset" as the offset argument to mmap here because
        // mmap's offset needs to be a multiple of the page size.

        // File might be empty (or the requested part of the file),
        // do not mmap in this case.
        if (bytes - offset <= 0)
            return;

        // We intend to read this.
        if ((p = mmap(NULL, bytes, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0)) == (void *)-1)
            eperror("mmap");
        
        if (madvise(p, bytes, MADV_WILLNEED | MADV_SEQUENTIAL) == -1)
            eperror("madvise");
        
        // Apply offset
        bytes -= offset;
        p = static_cast<void *>(static_cast<char *>(p) + offset);
    }

    ~MFile() {
        if (bytes - offset > 0) {
            bytes += offset;
            p = static_cast<void *>(static_cast<char *>(p) - offset);

            if (munmap(p, bytes) == -1)
                eperror("munmap");
        }

        if (close(fd) == -1)
            eperror("close");
    }

    const value_type* data() const { return static_cast<T*>(p); }
    value_type* data() { return static_cast<T*>(p); }

    size_t size() const { return bytes / sizeof(T); }

    const value_type* begin() const { return data(); }
    const value_type* end() const { return data() + size(); }

    const value_type& operator[](size_t i) const { return data()[i]; }
    value_type& operator[](size_t i){ return data()[i]; }

    int fd;
    off_t bytes, offset;
    void *p;
};

#endif
