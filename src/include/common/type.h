#pragma once

#include <cstdint>
#include 
namespace pidan {
using frame_id_t = int32_t;
using page_id_t = int32_t;
using timestamp_t = uint64_t;
using txn_id_t = uint64_t;

static constexpr page_id_t INVALID_PAGE_ID = -1;
static constexpr uintptr_t INVALID_VALUE_SLOT = 0;
static constexpr uint64_t INVALID_OLC_LOCK_VERSION = 0;
static constexpr size_t POINTER_SIZE = sizeof(void *);
static constexpr txn_id_t NULL_TXN_ID = 0;
static constexpr timestamp_t MAX_TIMESTAMP = UINT64_MAX;
static constexpr uint64_t NULL_DATA_LATCH = 0;
}