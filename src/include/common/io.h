#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "common/exception.h"

namespace pidan {

class PosixIOWrapper {
 public:
  PosixIOWrapper() = delete;

  template <class... Args>
  static int Open(const std::string &file, int oflag, Args... args) {
    while (true) {
      int ret = open(file.c_str(), oflag, args...);
      if (ret == -1) {
        if (errno == EINTR) continue;
        throw PosixError("Failed to open file with errno " + std::to_string(errno));
      }
      return ret;
    }
  }

  static void Close(int fd);

  static off_t lseek(int fd, off_t offset, int whence);

  static void WriteFully(int fd, const void *buffer, size_t nbytes);

  static uint32_t ReadFully(int fd, void *buffer, size_t nbytes);
};

}  // namespace pidan
