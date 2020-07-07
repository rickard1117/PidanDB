#include "common/io.h"

namespace pidan {

off_t PosixIOWrapper::lseek(int fd, off_t offset, int whence) {
  while (true) {
    int rc = lseek(fd, offset, whence);
    if (rc == -1) {
      if (errno == EINTR) continue;
      throw PosixError("Failed to lseek with errno " + std::to_string(errno));
    }
    return rc;
  }
}

void PosixIOWrapper::Close(int fd) {
  while (true) {
    int rc = close(fd);
    if (rc == -1) {
      if (errno == EINTR) continue;
      throw PosixError("Failed to lseek with errno " + std::to_string(errno));
    }
  }
}

void PosixIOWrapper::WriteFully(int fd, const void *buffer, size_t nbytes) {
  ssize_t total_written = 0;
  while (total_written < nbytes) {
    int rc = write(fd, reinterpret_cast<const char *>(buffer) + total_written, nbytes - total_written);
    if (rc == -1) {
      if (errno == EINTR) continue;
      throw PosixError("Failed to lseek with errno " + std::to_string(errno));
    }
    total_written += rc;
  }
}

uint32_t PosixIOWrapper::ReadFully(int fd, void *buffer, size_t nbytes) {
  ssize_t total_read = 0;
  while (total_read < nbytes) {
    int rc = read(fd, reinterpret_cast<char *>(buffer) + total_read, nbytes - total_read);
    if (rc == -1) {
      if (errno == EINTR) continue;
      throw PosixError("Failed to lseek with errno " + std::to_string(errno));
    }
    if (rc == 0) {
      break;
    }
    total_read += rc;
  }
  return static_cast<uint32_t>(total_read);
}

}  // namespace pidan