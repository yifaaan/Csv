#pragma once
#include <condition_variable>
#include <deque>
#include <string>
#include <optional>
#include <functional>
#include <utility>
#include "ConcurrentQueue.h"

namespace cl
{
	namespace csv
	{
		/// <summary>
		/// Ò»ÐÐ¼ÇÂ¼
		/// </summary>
		using RecordType = std::pair<unsigned, std::string>;

		class NotificationQueue
		{
			moodycamel::ConcurrentQueue<RecordType> queue;
			moodycamel::ProducerToken pToken{ queue };
			moodycamel::ConsumerToken cToken{ queue };

		public:
			bool TryDequeue(std::optional<RecordType>& op)
			{
				return this->queue.try_dequeue(this->cToken, op);
			}

			template<typename Record>
			bool Enqueue(Record&& record)
			{
				return this->queue.enqueue(this->pToken, std::forward<Record>(record));
			}
		};
	}
}