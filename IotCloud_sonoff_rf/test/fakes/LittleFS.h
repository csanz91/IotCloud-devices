#pragma once
#include <map>
#include <memory>
#include <string>
#include <cstring>
#include <algorithm>
class File {
    std::shared_ptr<std::string> data_;
    size_t position_ = 0;
public:
    File(std::shared_ptr<std::string> data = nullptr) : data_(data) {}
    explicit operator bool() const { return bool(data_); }
    size_t size() const { return data_->size(); }
    size_t write(uint8_t byte) { data_->push_back(byte); return 1; }
    size_t write(const uint8_t *p, size_t n) { data_->append((const char *)p, n); return n; }
    int read() { return position_ < size() ? uint8_t((*data_)[position_++]) : -1; }
    size_t readBytes(char *buffer, size_t n) {
        n = std::min(n, size() - position_);
        memcpy(buffer, data_->data() + position_, n); position_ += n; return n;
    }
    void flush() {}
    void close() {}
};
struct LittleFSConfig { void setAutoFormat(bool) {} };
struct FakeFS {
    std::map<std::string, std::shared_ptr<std::string>> files;
    bool mountable = true, failWrite = false, failRename = false;
    void setConfig(LittleFSConfig) {}
    bool begin() { return mountable; }
    void end() {}
    bool format() { files.clear(); mountable = true; return true; }
    bool exists(const char *path) { return files.count(path); }
    File open(const char *path, const char *mode) {
        if (*mode == 'w') {
            if (failWrite) return File{};
            files[path] = std::make_shared<std::string>();
        }
        return exists(path) ? File(files[path]) : File{};
    }
    bool rename(const char *from, const char *to) {
        if (failRename) return false;
        files[to] = files[from]; files.erase(from); return true;
    }
};
inline FakeFS LittleFS;
