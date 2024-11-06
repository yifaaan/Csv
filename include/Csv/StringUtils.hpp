#include <string>

namespace cl {
static inline void LTrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                  [](int ch) { return !std::isspace(ch); }));
}

static inline void RTrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

static inline void Trim(std::string& s) {
  LTrim(s);
  RTrim(s);
}

static inline std::string LTrimCopy(std::string s) {
  LTrim(s);
  return s;
}

static inline std::string RTrimCopy(std::string s) {
  RTrim(s);
  return s;
}

static inline std::string TrimCopy(std::string s) {
  Trim(s);
  return s;
}
} // namespace cl