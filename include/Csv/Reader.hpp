#pragma once
#include "StringUtils.hpp"
#include "TaskSystem.hpp"
#include <cstring>
#include <format>
#include <fstream>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
namespace cl {
namespace csv {
class Reader {
  friend class iterator;
  TaskSystem taskSystem{1};
  std::vector<std::string> header;
  size_t currentRow{0};

public:
  explicit Reader(std::string filename) {
    taskSystem.Start();
    auto file = std::ifstream{std::move(filename)};
    unsigned lineNo = 1;
    ReadFileFast(file, [&, this](char* buf, int length, int64_t position) {
      if (!buf)
        return;
      if (this->header.empty()) {
        header = ReadCsvRow(std::string(buf, length));
        // in-place RTrim the last header
        RTrim(this->header[this->header.size() - 1]);
        // for (const auto& h : header) {
        //   std::cout << std::format("{},", h);
        // }
        // std::cout << '\n';
        return;
      }
      this->taskSystem.Async(
          std::make_pair(lineNo++, std::string(buf, length)));
    });
    taskSystem.Stop();
    // std::cout << ">>>>>>>> Stopped\n";
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
    // std::cout << std::format("file size: {}\n", fileSize);
    file.seekg(0, std::ios::beg);
    bufSize = std::min(bufSize, fileSize);
    auto bufLength = bufSize;
    // 缓冲区
    auto buf = std::make_unique<char[]>(bufSize);
    file.read(buf.get(), bufSize);
    // std::cout << std::format("before process bufLength: {}\n", bufLength);
    // std::cout << std::format("buf:\n{}\n", buf.get());
    // 当前行的结尾
    int strEnd = -1;
    // 当前行的开头
    int strStart;
    // buf的开头在文件中的索引
    int64_t bufPositionInFile = 0;
    // 缓冲区还有数据
    while (bufLength > 0) {
      int i = strEnd + 1;
      // 指向上一行的结尾'\n'
      strStart = strEnd;
      strEnd = -1;
      // 找当前一行的结尾
      for (; i < bufLength && i + bufPositionInFile < fileSize; i++) {
        if (buf[i] == '\n') {
          strEnd = i;
          break;
        }
      }
      // std::cout << std::format("strStart: {}, strEnd: {}\n", strStart,
      // strEnd); 没找到换行符，即当前缓冲区的数据不是完整一行
      if (strEnd == -1) {
        // 当前行没有设置开头位置，则直接读取后续数据填充buf
        if (strStart == -1) {
          lineHandler(buf.get() + strStart + 1, bufLength,
                      bufPositionInFile + strStart + 1);
          bufPositionInFile += bufLength;
          bufLength = std::min(bufLength, fileSize - bufPositionInFile);
        } else {
          // 将当前末尾不含'\n'的部分行数据移动到缓冲区开头(即strStart+1..)
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
        // 找到完整的一行，就处理该行(包括'\n')
        lineHandler(buf.get() + strStart + 1, strEnd - strStart,
                    bufPositionInFile + strStart + 1);
      }
    }
    // eof
    lineHandler(nullptr, 0, 0);
  }

  enum class CSVState {
    // 正在处理未加引号的字段, 如 value1,value2
    UnquotedField,
    // 正在处理用引号包围的字段, 如 "value",
    // 引号内的内容可以包含逗号等其他字符，而不会被视为字段分隔符
    QuotedField,
    // 正在处理一个双引号字符内的引号, 如 "He said, ""Hello!""" 中的 ""Hello!""
    // 这部分内容
    QuotedQuote
  };

  std::vector<std::string> ReadCsvRow(const std::string& row) {
    auto state = CSVState::UnquotedField;
    auto fields = std::vector<std::string>{""};
    // index of current field
    size_t i = 0;
    for (char c : row) {
      switch (state) {
      case CSVState::UnquotedField:
        switch (c) {
        case ',': // end of field
          // generate a new field
          fields.emplace_back("");
          i++;
          break;
        case '"':
          state = CSVState::QuotedField;
          break;
        default:
          fields[i].push_back(c);
          break;
        }
        break;
      case CSVState::QuotedField:
        switch (c) {
        case '"':
          state = CSVState::QuotedQuote;
          break;
        default:
          fields[i].push_back(c);
          break;
        }
        break;
      case CSVState::QuotedQuote:
        switch (c) {
        case ',':
          fields.emplace_back("");
          i++;
          state = CSVState::UnquotedField;
          break;
        case '"':
          // 转译双引号
          fields[i].push_back('"');
          state = CSVState::QuotedField;
          break;
        default:
          state = CSVState::UnquotedField;
          break;
        }
        break;
      }
    }
    return fields;
  }

  std::optional<std::unordered_map<std::string_view, std::string>>
  operator[](size_t index) {
    if (auto it = this->taskSystem.rows.find(index);
        it != this->taskSystem.rows.end()) {
      auto row = ReadCsvRow(it->second);
      std::unordered_map<std::string_view, std::string> result;
      for (size_t i = 0; i < this->header.size(); i++) {
        if (i < row.size())
          result.emplace(this->header[i], row[i]);
        else
          result.emplace(this->header[i], "");
      }
      {
        const size_t i = this->header.size() - 1;
        if (i < row.size())
          result.emplace(this->header[i], RTrimCopy(row[i]));
        else
          result.emplace(this->header[i], "");
      }
      return result;
    }
    return std::nullopt;
  }
};
} // namespace csv
} // namespace cl