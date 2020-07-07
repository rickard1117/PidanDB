#include "assert.h"
#include "common/container/bitmap.h"
#include "storage/page/meta_page.h"

namespace pidan {
MetaPage::MetaPage() : Page() { 
    
    auto *bitmap = RawBitMap::CreateBitMap(GetData(), PAGE_SIZE, MAX_PAGE_NUM); 
    assert(bitmap != nullptr);
}
}  // namespace pidan