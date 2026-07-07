// UltraWeb/server/FileWatcher.h
// File change detection for incremental compilation
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Portable mtime-polling watcher (no inotify/kqueue dependency). Watches
// files and directories (recursive); PollChanges() reports paths that were
// added, modified, or removed since the previous poll. Start() runs the
// poll on a background thread and invokes a callback with the changes.

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace UltraWeb {
namespace Server {

class FileWatcher {
public:
    using ChangeCallback =
        std::function<void(const std::vector<std::string>& changedPaths)>;

    ~FileWatcher();

    // Watch a file, or a directory tree (recursive)
    void AddPath(const std::string& path);

    // Restrict directory scans to these extensions (with dot, e.g. ".css");
    // empty = all files
    void SetExtensionFilter(std::vector<std::string> extensions);

    // Returns paths added/modified/removed since the last poll.
    // The first poll establishes the baseline and reports nothing.
    std::vector<std::string> PollChanges();

    // Background polling
    void Start(uint32_t intervalMs, ChangeCallback callback);
    void Stop();
    bool IsRunning() const { return running; }

private:
    std::vector<std::string> watchedPaths;
    std::vector<std::string> extensions;
    std::map<std::string, int64_t> knownMtimes;  // path -> mtime (ns)
    bool baselineTaken = false;
    std::mutex mutex;

    std::atomic<bool> running{false};
    std::thread pollThread;

    void ScanInto(std::map<std::string, int64_t>& out) const;
    bool MatchesFilter(const std::string& path) const;
};

} // namespace Server
} // namespace UltraWeb
