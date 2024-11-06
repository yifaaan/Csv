#pragma once
#include "TaskSystem.hpp"
#include <cstring>
#include <fstream>
#include <memory>
#include <utility>

namespace cl {
namespace csv {
class Reader {
  TaskSystem taskSystem{1};

public:
  explicit Reader(std::string filename) {
    taskSystem.Start();
    auto file = std::ifstream{std::move(filename)};
    unsigned lineNo = 1;
    ReadFileFast(file, [&, this](char* buf, int length, int64_t position) {
      if (!buf)
        return;
      this->taskSystem.Async(
          std::make_pair(lineNo++, std::string(buf, length)));
    });
    taskSystem.Stop();
    std::cout << ">>>>>>>> Stopped\n";
  }

  /// 使用LineHandler来处理每一行
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
    auto bufLength = bufSize;
    // 缓冲区
    auto buf = std::make_unique<char[]>(bufSize);
    file.read(buf.get(), bufSize);

    // 当前行的结尾
    int strEnd = -1;
    // 当前行的开头
    int strStart;
    // buf的开头在文件中的索引
    int64_t bufPositionInFile = 0;
    // 缓冲区还有数据
    while (bufLength > 0) {
      int i = strEnd + 1;
      strStart = strEnd;
      strEnd = -1;
      // 找一行的结尾
      for (; i < bufLength && i + bufPositionInFile < fileSize; i++) {
        if (buf[i] == '\n') {
          strEnd = i;
          break;
        }
      }
      // 没找到换行符，即当前缓冲区的数据不是完整一行
      if (strEnd == -1) {
        // 当前行没有设置开头位置，则直接读取后续数据填充buf
        if (strStart == -1) {
          lineHandler(buf.get() + strStart + 1, bufLength,
                      bufPositionInFile + strStart + 1);
          bufPositionInFile += bufLength;
          bufLength = std::min(bufLength, fileSize - bufPositionInFile);
        } else {
          int movedLength = bufLength - strStart - 1;
          std::memmove(buf.get(), buf.get() + strStart + 1, movedLength);
          bufPositionInFile += strStart + 1;
          int readSize = std::min(bufLength - movedLength,
                                  fileSize - bufPositionInFile - movedLength);

          if (readSize != 0) {
            file.read(buf.get() + movedLength, readSize);
          }
          if (movedLength + readSize < bufLength) {
            auto tmp = std::make_unique<char[]>(movedLength + readSize);
            std::memmove(tmp.get(), buf.get(), movedLength + readSize);
            buf.swap(tmp);
            bufLength = movedLength + readSize;
          }
          strEnd = -1;
        }
      } else {
        // 找到完整的一行，就处理该行
        lineHandler(buf.get() + strStart + 1, strEnd - strStart,
                    bufPositionInFile + strStart + 1);
      }
    }
    // eof
    lineHandler(0, 0, 0);
  }
};
} // namespace csv
} // namespace cl