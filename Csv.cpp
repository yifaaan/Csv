// Csv.cpp: 定义应用程序的入口点。
//

#include "Csv.h"
#include "NotificationQueue.hpp"
#include "TaskSystem.hpp"
using namespace std;

int main()
{
	cout << "Hello CMake." << endl;
	auto queue = cl::csv::NotificationQueue{};
	auto ts = cl::csv::TaskSystem{};
	return 0;
}
