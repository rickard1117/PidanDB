#pragma once

#define DISALLOW_COPY(cname)     \
  cname(const cname &) = delete; \
  cname &operator=(const cname &) = delete;

#define DISALLOW_MOVE(cname) \
  cname(cname &&) = delete;  \
  cname &operator=(cname &&) = delete;

#define DISALLOW_COPY_AND_MOVE(cname) \
  DISALLOW_COPY(cname);               \
  DISALLOW_MOVE(cname);

// 声明一个类只允许通过raw memory以reinterpret_cast的形式转行而来，不允许通过任何形式构造函数进行创建。
#define MEM_REINTERPRET_CAST_ONLY(cname) \
  cname() = delete;                      \
  DISALLOW_COPY_AND_MOVE(cname);         \
  ~cname() = delete;
