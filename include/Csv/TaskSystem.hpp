#pragma once
#include "NotificationQueue.hpp"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace cl {
namespace csv {
using Lock = std::unique_lock<std::mutex>;
class TaskSystem {
  friend class Reader;
  const unsigned count;
  std::vector<std::thread> threads;
  std::vector<NotificationQueue> queues{count};
  std::atomic<unsigned> index{0};
  std::atomic_bool noMoreTasks{false};
  std::mutex rowsMutex;
  std::unordered_map<unsigned, std::string> rows;

  void Run(unsigned i) {
    while (true) {
      auto op = std::optional<RecordType>{};
      for (unsigned n = 0; n != this->count; n++) {
        if (this->queues[(i + n) % count].TryDequeue(op))
          break;
      }
      if (!op && !this->queues[i].TryDequeue(op)) {
        if (this->noMoreTasks) {
          break;
        } else {
          continue;
        }
      }

      {
        auto lock = Lock{this->rowsMutex};
        rows.insert(op.value());
      }
      std::cout << "Received row: " << op.value().first << " "
                << op.value().second << "\n";
    }
  }

public:
  explicit TaskSystem(
      const unsigned _count = std::thread::hardware_concurrency())
      : count(_count) {}

  ~TaskSystem() {
    std::ranges::for_each(this->threads, [](auto& thread) { thread.join(); });
  }

  void Start() {
    for (unsigned n = 0; n != this->count; n++) {
      this->threads.emplace_back([&, n] { Run(n); });
    }
  }

  void Stop() {
    std::cout << "Task system stopped\n";
    this->noMoreTasks = true;
  }

  template <typename Fn> void Async(Fn&& fn) {
    const auto i = this->index++;
    for (unsigned n = 0; n != this->count; n++) {
      if (this->queues[(i + n) % this->count].Enqueue(std::forward<Fn>(fn))) {
        return;
      }
    }
    this->queues[i % this->count].Enqueue(std::forward<Fn>(fn));
  }
};
} // namespace csv
} // namespace cl
