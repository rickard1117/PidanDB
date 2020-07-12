#pragma once

#include <cstdint>

namespace pidan {
using frame_id_t = int32_t;
using page_id_t = int32_t;

static constexpr page_id_t INVALID_PAGE_ID = -1;
static constexpr uintptr_t INVALID_VALUE_SLOT = 0;
static constexpr uint64_t INVALID_OLC_LOCK_VERSION = 0;
static constexpr size_t POINTER_SIZE = sizeof(void *);
}