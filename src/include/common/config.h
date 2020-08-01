#pragma once
#include <cstdint>

namespace pidan {

// 一个page的大小（字节）
static constexpr int PAGE_SIZE = 4096;

// 数据库Key的最大长度
static constexpr int MAX_KEY_SIZE = 256;

// 最大支持1TB的存储文件，最大存储page数量为1024 * 1024 * 1024 * 1024 / PAGE_SIZE = 268435456。
static constexpr uint32_t MAX_PAGE_NUM = 268435456;

// B+树节点分裂的阈值，当节点的填充率超过这个值时就需要分裂。
static constexpr int32_t BPLUSTREE_SPLIT_THREASHOLD = 80;

// B+树节点合并的阈值，当节点的填充率超过这个值时就需要合并。
static constexpr int32_t BPLUSTREE_MERGE_THRESHOLD = 20;

// 内存block大小，为1MB。
static constexpr int32_t BLOCK_SIZE = 1 << 20;

// B+树索引中内部节点的大小
static constexpr uint32_t BPLUSTREE_INNERNODE_SIZE = 4096;

// B+树索引中叶子节点的大小
static constexpr uint32_t BPLUSTREE_LEAFNODE_SIZE = 4096;

// B+树中两个epoch之间的间隔时间，单位毫秒
static constexpr uint32_t BPLUSTREE_EPOCH_INTERVAL = 100;

// B+树最多允许多少个不同的线程来访问
static constexpr int BPLUS_TREE_MAX_THREAD_NUM = 256;

// CPU中cache line大小
static constexpr int CACHE_LINE_SIZE = 64;

}