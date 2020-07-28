#pragma once

namespace pidan {

enum class Status {
  // 操作成功
  SUCCESS = 0,
  // key不存在
  KEY_NOT_EXIST = -1,
  // 因和正在执行的事务有冲突而失败
  FAIL_BY_ACTIVE_TXN = -2,
};

}