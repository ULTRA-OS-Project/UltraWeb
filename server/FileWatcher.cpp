// UltraWeb/server/FileWatcher.cpp
// Polling file watcher implementation
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "FileWatcher.h"

#include <algorithm>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

namespace UltraWeb {
namespace Server {

FileWatcher::~FileWatcher() { Stop(); }

void FileWatcher::AddPath(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex);
    if (std::find(watchedPaths.begin(), watchedPaths.end(), path) ==
        watchedPaths.end()) {
        watchedPaths.push_back(path);
        baselineTaken = false;  // re-baseline on next poll
    }
}

void FileWatcher::SetExtensionFilter(std::vector<std::string> exts) {
    std::lock_guard<std::mutex> lock(mutex);
    extensions = std::move(exts);
}

bool FileWatcher::MatchesFilter(const std::string& path) const {
    if (extensions.empty()) return true;
    for (const auto& ext : extensions) {
        if (path.size() >= ext.size() &&
            path.compare(path.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
    }
    return false;
}

void FileWatcher::ScanInto(std::map<std::string, int64_t>& out) const {
    std::error_code ec;
    for (const auto& root : watchedPaths) {
        fs::path p(root);
        if (fs::is_regular_file(p, ec)) {
            auto t = fs::last_write_time(p, ec);
            if (!ec) out[p.string()] = t.time_since_epoch().count();
        } else if (fs::is_directory(p, ec)) {
            for (auto it = fs::recursive_directory_iterator(
                     p, fs::directory_options::skip_permission_denied, ec);
                 it != fs::recursive_directory_iterator(); it.increment(ec)) {
                if (ec) break;
                if (!it->is_regular_file(ec)) continue;
                std::string filePath = it->path().string();
                if (!MatchesFilter(filePath)) continue;
                auto t = fs::last_write_time(it->path(), ec);
                if (!ec) out[filePath] = t.time_since_epoch().count();
            }
        }
    }
}

std::vector<std::string> FileWatcher::PollChanges() {
    std::lock_guard<std::mutex> lock(mutex);

    std::map<std::string, int64_t> current;
    ScanInto(current);

    std::vector<std::string> changed;
    if (baselineTaken) {
        // Added or modified
        for (const auto& kv : current) {
            auto it = knownMtimes.find(kv.first);
            if (it == knownMtimes.end() || it->second != kv.second) {
                changed.push_back(kv.first);
            }
        }
        // Removed
        for (const auto& kv : knownMtimes) {
            if (!current.count(kv.first)) changed.push_back(kv.first);
        }
    }

    knownMtimes = std::move(current);
    baselineTaken = true;
    return changed;
}

void FileWatcher::Start(uint32_t intervalMs, ChangeCallback callback) {
    if (running) return;
    running = true;
    PollChanges();  // establish baseline before the thread starts

    pollThread = std::thread([this, intervalMs, callback]() {
        while (running) {
            // Sleep in short slices so Stop() returns promptly even with
            // long poll intervals
            for (uint32_t slept = 0; running && slept < intervalMs; slept += 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    std::min<uint32_t>(50, intervalMs - slept)));
            }
            if (!running) break;
            auto changes = PollChanges();
            if (!changes.empty() && callback) callback(changes);
        }
    });
}

void FileWatcher::Stop() {
    if (!running) return;
    running = false;
    if (pollThread.joinable()) pollThread.join();
}

} // namespace Server
} // namespace UltraWeb
