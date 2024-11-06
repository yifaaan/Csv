#pragma once
#include "ConcurrentQueue.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace cl {
namespace csv {
/// <summary>
/// һ�м�¼
/// </summary>
using RecordType = std::pair<unsigned, std::string>;

class NotificationQueue {
  moodycamel::ConcurrentQueue<RecordType> queue;
  moodycamel::ProducerToken pToken{queue};
  moodycamel::ConsumerToken cToken{queue};

public:
  bool TryDequeue(std::optional<RecordType>& op) {
    return this->queue.try_dequeue(this->cToken, op);
  }

  template <typename Record> bool Enqueue(Record&& record) {
    return this->queue.enqueue(this->pToken, std::forward<Record>(record));
  }
};
} // namespace csv
} // namespace cl