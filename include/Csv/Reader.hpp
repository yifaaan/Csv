#pragma once
#include "TaskSystem.hpp"
#include <fstream>
#include <memory>

namespace cl {
namespace csv {
class Reader {
  TaskSystem taskSystem{1};

public:
  explicit Reader(std::string filename) {
    taskSystem.Start();
    auto file = std::ifstream{std::move(filename)};
    std::string line;
    unsigned lineNo = 1;
    while (std::getline(file, line)) {
      taskSystem.Async(std::make_pair(lineNo, line));
      lineNo++;
    }
    taskSystem.Stop();
    std::cout << ">>>>>>>> Stopped\n";
  }

  template <typename LineHandler>
  void ReadFileFast(std::ifstream& file, LineHandler&& lineHandler) {
    int64_t bufSize = 40000;
    // Seek read pointer to file end
    file.seekg(0, std::ios::end);
    // Get read pointer(file size)
    auto p = file.tellg();
    int64_t fileSize = p;
    file.seekg(0, std::ios::beg);
    bufSize = std::min(bufSize, fileSize);
    auto buf = std::make_unique<char[]>(bufSize);
    file.read(buf.get(), bufSize);

    int strEnd = -1;
    int strStart;
    int64_t bufPositionInFile = 0;
    while (bufSize > 0) {
      int i = strEnd + 1;
      strStart = strEnd;
      strEnd = -1;
      for (; i < bufSize && i + bufPositionInFile < fileSize; i++) {
      }
    }
  }
};
} // namespace csv
} // namespace cl