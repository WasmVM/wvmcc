#pragma once

#include <string>
#include <vector>
#include <utility>

namespace wvmcc {

// Registry of source files seen during preprocessing (#28, approach B). Every
// opened file — the primary TU and each #include — is registered once and gets
// a stable, nonzero fileId that is stamped into every token's SourcePos. The
// diagnostic printer later recovers a file's display path and text by fileId, so
// a caret can be rendered for an error anywhere in the include tree.
//
// A single instance is shared (via shared_ptr) by a Preprocessor and all the
// child Preprocessors it spawns for #includes, so ids never collide and the
// registry outlives the (transient) child preprocessors.
class SourceManager {
public:
    // Register `path` with its full `text`; returns the assigned fileId (>=1).
    // The text is kept and split into lines lazily, only if a diagnostic in this
    // file actually needs a caret.
    int addFile(std::string path, std::string text) {
        int id = static_cast<int>(files_.size()); // index 0 is the sentinel below
        files_.push_back(Entry{std::move(path), std::move(text), {}, false});
        return id;
    }

    // fileId -> display path, or nullptr if unknown (including the reserved 0).
    const std::string* pathForId(int fileId) const {
        if (fileId <= 0 || fileId >= static_cast<int>(files_.size())) return nullptr;
        return &files_[fileId].path;
    }

    // fileId -> source lines (line endings stripped), or nullptr if unknown.
    const std::vector<std::string>* linesForId(int fileId) const {
        if (fileId <= 0 || fileId >= static_cast<int>(files_.size())) return nullptr;
        const Entry& e = files_[fileId];
        if (!e.split) { splitLines(e.text, e.lines); e.split = true; }
        return &e.lines;
    }

private:
    struct Entry {
        std::string path;
        std::string text;
        mutable std::vector<std::string> lines; // populated lazily by linesForId
        mutable bool split = false;
    };
    // Index 0 is a reserved sentinel so fileId 0 keeps meaning "unknown".
    std::vector<Entry> files_{ Entry{} };

    static void splitLines(const std::string& text, std::vector<std::string>& out) {
        std::string line;
        for (char c : text) {
            if (c == '\n') {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                out.push_back(std::move(line));
                line.clear();
            } else {
                line.push_back(c);
            }
        }
        if (!line.empty()) {
            if (line.back() == '\r') line.pop_back();
            out.push_back(std::move(line));
        }
    }
};

} // namespace wvmcc
