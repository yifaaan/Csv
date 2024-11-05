#pragma once
#include <condition_variable>
#include <deque>
#include <string>
#include <optional>
#include <functional>
#include <utility>

namespace cl
{
	namespace csv
	{
		using LockType = std::unique_lock<std::mutex>;
		/// <summary>
		/// Ò»ÐÐ¼ÇÂ¼
		/// </summary>
		using RecordType = std::pair<unsigned, std::string>;

		class NotificationQueue
		{
			std::deque<RecordType> queue;
			bool done{ false };
			std::mutex mutex;
			std::condition_variable ready;

		public:
			bool TryPop(std::optional<RecordType>& op)
			{
				auto lock = LockType{ this->mutex, std::try_to_lock };
				if (!lock || this->queue.empty())
				{
					return false;
				}
				op = std::move(queue.front());
				queue.pop_front();
				return true;
			}

			template<typename Fn>
			bool TryPush(Fn&& fn)
			{
				{
					auto lock = LockType{ this->mutex, std::try_to_lock };
					if (!lock)
					{
						return false;
					}
					this->queue.emplace_back(std::forward<Fn>(fn));
				}
				this->ready.notify_one();
				return true;
			}

			void Done()
			{
				{
					auto lock = LockType{ this->mutex };
					this->done = true;
				}
				this->ready.notify_all();
			}

			bool Pop(RecordType& op)
			{
				auto lock = LockType{ this->mutex };
				this->ready.wait(lock, [this]()
				{
						return !this->queue.empty();
				});
				if (this->queue.empty())
				{
					return false;
				}
				op = std::move(this->queue.front());
				this->queue.pop_front();
				return true;
			}

			template<typename Fn>
			void Push(Fn&& fn)
			{
				{
					auto lock = LockType{ this->mutex };
					this->queue.emplace_back(std::function<Fn>(fn));
				}
				this->ready.notify_one();
			}
		};
	}
}