// cog/plan9/expression_fs.hpp — Plan 9 Cognitive Namespace Expression File Server
// Exposes MetaHuman DNA expression state as a 9P2000 synthetic filesystem
// Header-only, C++11, zero external dependencies
// SPDX-License-Identifier: MIT
//
// Namespace layout:
//   /mnt/expression/
//   ├── ctl           — Control file (write: "reset", "chaos on/off", etc.)
//   ├── facs          — Read: current FACS AU activations (text)
//   ├── morph         — Read: MetaHuman CTRL_ morph targets (text)
//   ├── material      — Read: dynamic material parameters (text)
//   ├── endocrine     — Read: hormone concentrations (text)
//   ├── chaos         — Read: Lorenz attractor state (text)
//   ├── aesthetic     — Read/Write: SuperHotGirl parameters (text)
//   ├── frame         — Read: current frame number (text)
//   └── replay/       — Directory of recorded frames
//       ├── 000001    — Frame 1 JSON
//       ├── 000002    — Frame 2 JSON
//       └── ...
//
#ifndef COG_PLAN9_EXPRESSION_FS_HPP
#define COG_PLAN9_EXPRESSION_FS_HPP

#include "../core/core.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <functional>

namespace cog { namespace plan9 {

// ─────────────────────────────────────────────────────────────────────────────
// File Types in the expression namespace
// ─────────────────────────────────────────────────────────────────────────────
enum class ExprFileType : uint8_t {
    CTL,        // Control file
    FACS,       // FACS AU state
    MORPH,      // Morph targets
    MATERIAL,   // Material params
    ENDOCRINE,  // Hormone state
    CHAOS,      // Lorenz state
    AESTHETIC,  // Aesthetic params
    FRAME,      // Frame counter
    REPLAY_DIR, // Replay directory
    REPLAY_ENTRY // Individual replay frame
};

// ─────────────────────────────────────────────────────────────────────────────
// ExprFile — A synthetic file in the expression namespace
// ─────────────────────────────────────────────────────────────────────────────
struct ExprFile {
    std::string name;
    ExprFileType type;
    uint32_t mode;   // Unix-style permissions
    bool is_dir;

    // Read callback: returns file content as string
    std::function<std::string()> read_fn;
    // Write callback: processes written data
    std::function<bool(const std::string&)> write_fn;

    ExprFile() : type(ExprFileType::CTL), mode(0444), is_dir(false) {}
    ExprFile(const std::string& n, ExprFileType t, uint32_t m, bool d = false)
        : name(n), type(t), mode(m), is_dir(d) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// ExpressionFileServer — 9P-style synthetic filesystem for expression state
// ─────────────────────────────────────────────────────────────────────────────
class ExpressionFileServer {
public:
    ExpressionFileServer() {
        // Build the namespace
        add_file("ctl", ExprFileType::CTL, 0666);
        add_file("facs", ExprFileType::FACS, 0444);
        add_file("morph", ExprFileType::MORPH, 0444);
        add_file("material", ExprFileType::MATERIAL, 0444);
        add_file("endocrine", ExprFileType::ENDOCRINE, 0444);
        add_file("chaos", ExprFileType::CHAOS, 0444);
        add_file("aesthetic", ExprFileType::AESTHETIC, 0666);
        add_file("frame", ExprFileType::FRAME, 0444);
        add_dir("replay");
    }

    // Register read callback for a file
    void on_read(const std::string& name, std::function<std::string()> fn) {
        auto it = files_.find(name);
        if (it != files_.end()) {
            it->second.read_fn = fn;
        }
    }

    // Register write callback for a file
    void on_write(const std::string& name,
                  std::function<bool(const std::string&)> fn) {
        auto it = files_.find(name);
        if (it != files_.end()) {
            it->second.write_fn = fn;
        }
    }

    // 9P Tread — Read a file
    std::string read(const std::string& path) const {
        auto it = files_.find(path);
        if (it == files_.end()) return ""; // Rerror
        if (it->second.read_fn) return it->second.read_fn();
        return "";
    }

    // 9P Twrite — Write to a file
    bool write(const std::string& path, const std::string& data) {
        auto it = files_.find(path);
        if (it == files_.end()) return false;
        if (!(it->second.mode & 0222)) return false; // Permission denied
        if (it->second.write_fn) return it->second.write_fn(data);
        return false;
    }

    // 9P Tstat — Get file metadata
    std::string stat(const std::string& path) const {
        auto it = files_.find(path);
        if (it == files_.end()) return "";
        std::ostringstream ss;
        ss << it->second.name << " "
           << std::oct << it->second.mode << std::dec << " "
           << (it->second.is_dir ? "d" : "-");
        return ss.str();
    }

    // 9P Twalk — List directory contents
    std::vector<std::string> walk(const std::string& dir = "") const {
        std::vector<std::string> entries;
        for (const auto& kv : files_) {
            if (dir.empty() || kv.first.find(dir) == 0) {
                entries.push_back(kv.first);
            }
        }
        return entries;
    }

    // Add a replay frame
    void add_replay_frame(uint64_t frame_num, const std::string& json) {
        std::ostringstream name;
        name << "replay/" << std::setfill('0') << std::setw(6) << frame_num;
        std::string path = name.str();
        ExprFile f(path, ExprFileType::REPLAY_ENTRY, 0444);
        std::string captured_json = json;
        f.read_fn = [captured_json]() { return captured_json; };
        files_[path] = f;
    }

    size_t file_count() const { return files_.size(); }

    // Format FACS state as Plan 9 text
    static std::string format_facs(const float au_values[20]) {
        static const char* names[] = {
            "AU1", "AU2", "AU4", "AU5", "AU6", "AU7", "AU9", "AU10",
            "AU12", "AU14", "AU15", "AU17", "AU20", "AU23", "AU25",
            "AU26", "AU28", "AU43", "AU45", "AU46"
        };
        std::ostringstream ss;
        for (int i = 0; i < 20; ++i) {
            if (au_values[i] > 0.001f) {
                ss << names[i] << "\t" << std::fixed << std::setprecision(3)
                   << au_values[i] << "\n";
            }
        }
        return ss.str();
    }

private:
    std::unordered_map<std::string, ExprFile> files_;

    void add_file(const std::string& name, ExprFileType type, uint32_t mode) {
        files_[name] = ExprFile(name, type, mode, false);
    }

    void add_dir(const std::string& name) {
        files_[name] = ExprFile(name, ExprFileType::REPLAY_DIR, 0555, true);
    }
};

}} // namespace cog::plan9

#endif // COG_PLAN9_EXPRESSION_FS_HPP
