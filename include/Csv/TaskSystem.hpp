#pragma once
#include <atomic>
#include <vector>
#include <thread>
#include <unordered_map>
#include <iostream>
#include <ranges>
#include <algorithm>
#include "NotificationQueue.hpp"

namespace cl
{
	namespace csv
	{
		class TaskSystem
		{
			const unsigned count{ std::thread::hardware_concurrency() };
			std::vector<std::thread> threads;
			std::vector<NotificationQueue> queues{ count };
			std::atomic<unsigned> index{ 0 };
			std::atomic_bool done{ false };
			std::mutex queueMutex;
			std::condition_variable ready;
			std::mutex rowsMutex;
			std::unordered_map<unsigned, std::string> rows;

			/// <summary>
			/// 每个线程都在执行该操作
			/// </summary>
			/// <param name="i">线程编号</param>
			void Run(unsigned i)
			{
				while (true)
				{
					auto op = std::optional<RecordType>{};
					for (unsigned n = 0; n != this->count; n++)
					{
						if (this->queues[(i + n) % count].TryPop(op)) break;
					}
					if (!op && !this->queues[i].TryPop(op))
					{
						if (this->done)
						{
							break;
						}
						else
						{
							continue;
						}
					}
					// Use RecordType op
					{
						auto lock = LockType{ this->rowsMutex };
						rows.insert(op.value());
					}
					std::cout << "Received row: " << op.value().first << " " << op.value().second << "\n";
				}
			}

		public:
			explicit TaskSystem(const unsigned _count = std::thread::hardware_concurrency())
				: count(_count)
			{
				std::cout << this->count << '\n';
			}

			~TaskSystem()
			{
				std::ranges::for_each(this->queues, [](auto& queue)
				{
					queue.Done();
				});
				std::ranges::for_each(this->threads, [](auto& thread)
				{
					thread.join();
				});
				std::cout << this->rows.size() << std::endl;
			}

			void Start()
			{
				for (unsigned n = 0; n != this->count; n++)
				{
					this->threads.emplace_back([&, n] { Run(n); });
				}
			}

			void Stop()
			{
				std::cout << "Task system stopped\n";
				this->done = true;
			}

			template<typename Fn>
			void Async(Fn&& fn)
			{
				while (!this->done)
				{
					auto i = this->index++;
					for (unsigned n = 0; n != this->count; n++)
					{
						if (this->queues[i % this->count].TryPush(std::forward<Fn>(fn)))
						{
							return;
						}
						this->index = 0;
					}
				}
			}
		};
	}
}
