#pragma once

#include <atomic>
#include <fstream>
#include <functional>
#include <thread>

#include "common/macros.h"
#include "common/type.h"
#include "container/bplustree/node.h"

namespace pidan {

// 一个垃圾节点，等待被GC的回收
struct GarbageNode {
  Node *node{nullptr};
  GarbageNode *next{nullptr};
};

// 一个EpochNode存储了一个Epoch周期中待回收的垃圾数据
struct EpochNode {
  // 当前Epoch中活跃的线程数
  std::atomic<uint64_t> active_thread_count{0};
  // 当前Epoch中待回收的垃圾节点链表
  std::atomic<GarbageNode *> garbage_list{nullptr};
  // 下一个EpochNode
  EpochNode *next{nullptr};
};

class EpochManager {
 public:
  DISALLOW_COPY_AND_MOVE(EpochManager);

  EpochManager() : head_epoch_(new EpochNode()), current_epoch_(head_epoch_){};

  ~EpochManager() { Stop(); }

  // 启动一个线程，不断地去更新epoch
  void Start() {
    terminate_.store(false);
    thread_ = new std::thread([this] {
      while (!terminate_) {
        this->PerformGC();
        this->CreateNewEpoch();
        std::this_thread::sleep_for(std::chrono::milliseconds(BPLUSTREE_EPOCH_INTERVAL));
      }
    });
  }

  void Stop() {
    terminate_.store(true);
    if (thread_ != nullptr) {
      thread_->join();
      delete thread_;
    }
  }

  EpochNode *JoinEpoch() {
    // 我们必须保证join的Epoch和leave的Epoch是同一个
    EpochNode *epoch = current_epoch_;
    current_epoch_->active_thread_count.fetch_add(1);
    return current_epoch_;
  }

  void LeaveEpoch(EpochNode *epoch) { epoch->active_thread_count.fetch_sub(1); }

  // 增加一个待回收的Node,会在其他线程调用
  void AddGarbageNode(Node *node) {
    // 这里要copy一份current_epoch_的复制，因为GC线程可能会调用CreateNewEpoch
    EpochNode *epoch = current_epoch_;
    auto *garbage = new GarbageNode();
    garbage->node = node;
    garbage->next = epoch->garbage_list.load();
    for (;;) {
      auto result = epoch->garbage_list.compare_exchange_strong(garbage->next, garbage);
      if (result) {
        break;
      }
      // 如果CAS失败，那么garbage->next会更新为epoch->garbage_list的新值，则继续重试就好了。
    }
  }

  void PerformGC() {
    for (;;) {
      if (head_epoch_ == current_epoch_) {
        // 我们至少要保留一个epoch
        return;
      }
      // 从head开始遍历epoch node链表
      if (head_epoch_->active_thread_count.load() > 0) {
        return;
      }

      // 已经没有线程在这个epoch上了，可以直接释放它所有的garbage节点。
      while (head_epoch_->garbage_list != nullptr) {
        GarbageNode *garbage_node = head_epoch_->garbage_list.load();
        delete garbage_node->node;
        head_epoch_->garbage_list = head_epoch_->garbage_list.load()->next;
        delete garbage_node;
#ifndef NDEBUG
        garbage_node_del_num_.fetch_add(1);
#endif
      }
      EpochNode *epoch = head_epoch_;
      head_epoch_ = head_epoch_->next;
      delete epoch;
#ifndef NDEBUG
      epoch_del_num_.fetch_add(1);
#endif
    }
  }

#ifndef NDEBUG
  uint32_t GarbageNodeDelNum() {
    return garbage_node_del_num_.load();
  } 
#endif

  void CreateNewEpoch() {
    auto *new_epoch = new EpochNode();
    current_epoch_->next = new_epoch;
    current_epoch_ = new_epoch;
  }

 private:
  // Epoch链表的头结点，不需要atomic因为只有GC线程会修改和读取它
  EpochNode *head_epoch_;
  // 当前Epoch所在的节点，不需要atomic因为只有GC线程会修改，业务线程虽然会读取，但是读到旧数据也没关系
  EpochNode *current_epoch_;
  std::atomic<bool> terminate_{true};
  std::thread *thread_{nullptr};
#ifndef NDEBUG
  std::atomic<uint32_t> garbage_node_del_num_{0};  // 删除的垃圾节点的数量
  std::atomic<uint32_t> epoch_del_num_{0};         // 删除的epoch节点的数量
#endif
};

// 支持变长key，定长value，非重复key的线程安全B+树。
template <typename KeyType, typename ValueType>
class BPlusTree {
 public:
  BPlusTree() : root_(new LNode) { }

  // 查找key对应的value，找到返回true，否则返回false。
  bool Lookup(const KeyType &key, ValueType *value) const {
    for (;;) {
      Node *node = root_.load();
      bool need_restart = false;
      EpochNode *epoch = epoch_manager_.JoinEpoch();
      bool result = StartLookup(node, nullptr, INVALID_OLC_LOCK_VERSION, key, value, &need_restart);
      epoch_manager_.LeaveEpoch(epoch);
      if (need_restart) {
        // 查找失败，需要重启整个流程。
        continue;
      }
      return result;
    }
  }

  // 插入一对key value，要求key是唯一的。如果key已经存在则返回false，并将value设置为已经存在的值。
  // 插入成功返回true，不对value做任何改动。
  bool InsertUnique(const KeyType &key, const ValueType &value, ValueType *old_val) {
    for (;;) {
      Node *node = root_.load();
      bool need_restart = false;
      EpochNode *epoch = epoch_manager_.JoinEpoch();
      bool result = StartInsertUnique(node, nullptr, INVALID_OLC_LOCK_VERSION, key, value, old_val, &need_restart);
      epoch_manager_.LeaveEpoch(epoch);
      if (need_restart) {
        continue;
      }
      return result;
    }
  }

  // 查找key，如果找到，返回true并返回对应的Value值。如果没找到则返回false并构造一个新的value。
  bool CreateIfNotExist(const KeyType &key, ValueType *new_val, const std::function<ValueType(void)> &creater) {
    for (;;) {
    }
  }

  // 只是测试用
  void DrawTreeDot(const std::string &filename) {
    std::ofstream out(filename);
    out << "digraph g { " << '\n';
    out << "node [shape = record,height=0.1];" << '\n';
    IDGenerator g;
    draw_node(root_, out, g.gen(), g);
    out << "}" << '\n';
  }

 private:
  using LNode = LeafNode<KeyType, ValueType>;
  using INode = InnerNode<KeyType>;

  class IDGenerator {
   public:
    IDGenerator() : id_(0) {}
    size_t gen() { return id_++; }

   private:
    size_t id_;
  };

  void draw_node(Node *node, std::ofstream &ofs, size_t my_id, IDGenerator &g) {
    std::string my_id_s = std::to_string(my_id);
    ofs.flush();
    if (!node->IsLeaf()) {
      INode *inner = static_cast<INode *>(node);
      ofs << "node" << my_id_s << "[label = \"";
      for (uint16_t i = 0; i < inner->key_map_.size(); i++) {
        ofs << "<f" << std::to_string(i) << ">";
        ofs << "|" << inner->key_map_.KeyAt(i).ToString() << "|";
      }
      ofs << "<f" << inner->key_map_.size() << ">\"];\n";

      size_t childid = g.gen();
      ofs << "\"node" << my_id_s << "\""
          << ":f"
          << "0"
          << "->"
          << "\"node" << std::to_string(childid) << "\"\n";
      draw_node(inner->first_child_, ofs, childid, g);

      for (uint16_t i = 0; i < inner->key_map_.size(); i++) {
        childid = g.gen();
        ofs << "\"node" << my_id_s << "\""
            << ":f" << std::to_string(i + 1) << "->"
            << "\"node" << std::to_string(childid) << "\"\n";
        draw_node(inner->key_map_.ValueAt(i), ofs, childid, g);
      }
      return;
    }

    LNode *leaf = static_cast<LNode *>(node);
    for (uint16_t i = 0; i < leaf->key_map_.size(); i++) {
      if (i > 0) {
        ofs << "|";
      }
      ofs << leaf->key_map_.KeyAt(i).ToString();
    }
    ofs << "\"];\n";
  }

  // 从node节点开始，向树中插入key value，插入失败返回false，否则返回true。
  bool StartInsertUnique(Node *node, INode *parent, uint64_t parent_version, const KeyType &key, const ValueType &val,
                         ValueType *old_val, bool *need_restart) {
    uint64_t version;
    if (!node->ReadLockOrRestart(&version)) {
      *need_restart = true;
      return false;
    }

    if (!node->IsLeaf()) {
      INode *inner = static_cast<INode *>(node);
      if (!inner->EnoughSpaceFor(MAX_KEY_SIZE)) {
        // 节点空间不足，要分裂。
        if (parent) {
          if (!parent->UpgradeToWriteLockOrRestart(parent_version)) {
            *need_restart = true;
            return false;
          }
        }

        if (!inner->UpgradeToWriteLockOrRestart(version)) {
          if (parent) {
            parent->WriteUnlock();
          }
          *need_restart = true;
          return false;
        }

        // TODO: 这个地方有疑问，到底应不应该判断。
        if (parent == nullptr && (inner != root_.load())) {
          // node原本是根节点，但是同时有其他线程在此线程对根节点加写锁之前已经将根节点分裂或删除了
          // 此时虽然加写锁可以成功，但根节点已经是新的节点了，因此要重启。
          inner->WriteUnlock();
          *need_restart = true;
          return false;
        }

        KeyType split_key;
        INode *sibling = inner->Split(&split_key);
        if (parent) {
          bool result = parent->Insert(split_key, sibling);
          assert(result);
        } else {
          root_ = new INode(inner->level() + 1, inner, sibling, split_key);
        }
        inner->WriteUnlock();
        if (parent) {
          parent->WriteUnlock();
        }
        // 分裂完毕，重新开始插入流程。
        *need_restart = true;
        return false;
      }

      if (parent) {
        if (!parent->ReadUnlockOrRestart(parent_version)) {
          *need_restart = true;
          return false;
        }
      }

      Node *child = inner->FindChild(key);
      if (!inner->CheckOrRestart(version)) {
        *need_restart = true;
        return false;
      }

      return StartInsertUnique(child, inner, version, key, val, old_val, need_restart);
    }

    LNode *leaf = static_cast<LNode *>(node);
    if (leaf->Exists(key, old_val)) {
      if (!leaf->ReadUnlockOrRestart(version)) {
        *need_restart = true;
      } else {
        *need_restart = false;
      }
      return false;
    }

    if (!leaf->EnoughSpaceFor(MAX_KEY_SIZE)) {
      if (parent) {
        // leaf节点要分裂，会向父节点插入key，要先拿到父节点的写锁。
        // 之前访问父节点已经保证了父节点的空间足够，如果在访问后父节点发生了改动，那么这里会加锁失败。
        if (!parent->UpgradeToWriteLockOrRestart(parent_version)) {
          *need_restart = true;
          return false;
        }
      }

      if (!leaf->UpgradeToWriteLockOrRestart(version)) {
        *need_restart = true;
        if (parent) {
          parent->WriteUnlock();
        }
        return false;
      }

      // TODO: 这个地方有疑问，到底应不应该判断。
      if (parent == nullptr && (node != root_)) {
        // node原本是根节点，但是同时有其他线程在此线程对根节点加写锁之前已经将根节点分裂或删除了
        // 此时虽然加写锁可以成功，但根节点已经是新的节点了，因此要重启。
        leaf->WriteUnlock();
        *need_restart = true;
        return false;
      }

      KeyType split_key;
      LNode *sibling = leaf->Split(&split_key);
      if (parent) {
        bool result = parent->Insert(split_key, sibling);
        assert(result);
      } else {
        // 当前节点是leaf node，那么父节点的level必须是1
        root_ = new INode(1, leaf, sibling, split_key);
      }
      // TODO : 分裂完毕了，此时是否可以直接将key插入到leaf或者sibling节点了，还是需要再重启一次？
      // if (key < split_key) {
      //   leaf->Insert(key, val);
      // } else {
      //   sibling->Insert(key, val);
      // }
      leaf->WriteUnlock();
      if (parent) {
        parent->WriteUnlock();
      }
      *need_restart = true;
      return false;
    } else {
      // leaf 节点空间足够，直接插入不需要再对父节点加写锁了
      if (!leaf->UpgradeToWriteLockOrRestart(version)) {
        *need_restart = true;
        return false;
      }
      if (parent) {
        if (!parent->ReadUnlockOrRestart(parent_version)) {
          *need_restart = true;
          leaf->WriteUnlock();
          return false;
        }
      }
      leaf->Insert(key, val);
      leaf->WriteUnlock();
      return true;
    }
  }

  // 从节点node开始查找key，没有找到则返回false
  bool StartLookup(const Node *node, const Node *parent, const uint64_t parent_version, const KeyType &key,
                   ValueType *val, bool *need_restart) const {
    // a_.fetch_add(1);
    uint64_t version;
    if (!node->ReadLockOrRestart(&version)) {
      *need_restart = true;
      return false;
    }

    if (parent) {
      if (!parent->ReadUnlockOrRestart(parent_version)) {
        *need_restart = true;
        return false;
      }
    }

    if (node->IsLeaf()) {
      const LNode *leaf = static_cast<const LNode *>(node);
      bool result = leaf->FindValue(key, val);
      if (!leaf->ReadUnlockOrRestart(version)) {
        *need_restart = true;
        return false;
      }
      *need_restart = false;
      return result;
    }

    const INode *inner = static_cast<const INode *>(node);
    const Node *child = inner->FindChild(key);
    assert(child != nullptr);
    // 这里需要再次检查，以保证child指针的有效性。
    if (!inner->CheckOrRestart(version)) {
      *need_restart = true;
      return false;
    }
    return StartLookup(child, node, version, key, val, need_restart);
  }

 private:
  std::atomic<Node *> root_;
  mutable EpochManager epoch_manager_;
};

// 画出树的dot图，仅用于测试
void DrawTreeDot(const std::string &filename);

}  // namespace pidan