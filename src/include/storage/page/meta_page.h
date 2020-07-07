#pragma once

#include "storage/page/page.h"

namespace pidan {

/** 数据库的MetaPage，存放在数据库文件的最头部。布局如下：
 *  ----------------------------------------------------------------------------
 *  RawBitMap()
 *  ----------------------------------------------------------------------------
 */
class RawBitMap;
class MetaPage : public Page {
 public:
  MetaPage();

  RawBitMap *GetBitMap() { return reinterpret_cast<RawBitMap *>(GetData()); }
};

}  // namespace pidan