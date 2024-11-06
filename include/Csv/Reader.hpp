#pragma once
#include "TaskSystem.hpp"

namespace cl
{
	namespace csv
	{
		class Reader
		{
			TaskSystem taskSystem{ 1 };
		public:
			explicit Reader(std::string filename)
			{
				taskSystem.Start();
				auto file = std::ifstream{ std::move(filename) };
				std::string line;
				unsigned lineNo = 1;
				while (std::getline(file, line))
				{
					taskSystem.Async(std::make_pair(lineNo, line));
					lineNo++;
				}
				taskSystem.Stop();
				std::cout << ">>>>>>>> Stopped\n";
			}
		};
	}
}