// cog/plan9/plan9.hpp — Plan 9 Cognitive OS: 9P protocol, CogFS, MachSpace
// Header-only, C++11, zero external dependencies
// SPDX-License-Identifier: MIT
#ifndef COG_PLAN9_HPP
#define COG_PLAN9_HPP

#include "../core/core.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <algorithm>

namespace cog { namespace plan9 {

// ─────────────────────────────────────────────────────────────────────────────
// 9P2000 Message Types
// ─────────────────────────────────────────────────────────────────────────────
enum class MsgType : uint8_t {
    Tversion = 100, Rversion = 101,
    Tauth    = 102, Rauth    = 103,
    Tattach  = 104, Rattach  = 105,
    Terror   = 106, Rerror   = 107,
    Tflush   = 108, Rflush   = 109,
    Twalk    = 110, Rwalk    = 111,
    Topen    = 112, Ropen    = 113,
    Tcreate  = 114, Rcreate  = 115,
    Tread    = 116, Rread    = 117,
    Twrite   = 118, Rwrite   = 119,
    Tclunk   = 120, Rclunk   = 121,
    Tremove  = 122, Rremove  = 123,
    Tstat    = 124, Rstat    = 125,
    Twstat   = 126, Rwstat   = 127
};

// ─────────────────────────────────────────────────────────────────────────────
// Qid — Unique file identification
// ─────────────────────────────────────────────────────────────────────────────
struct Qid {
    uint8_t  type;    // QTDIR, QTFILE, etc.
    uint32_t version;
    uint64_t path;

    static constexpr uint8_t QTDIR    = 0x80;
    static constexpr uint8_t QTAPPEND = 0x40;
    static constexpr uint8_t QTEXCL   = 0x20;
    static constexpr uint8_t QTAUTH   = 0x08;
    static constexpr uint8_t QTFILE   = 0x00;
};

// ─────────────────────────────────────────────────────────────────────────────
// Stat — File metadata
// ─────────────────────────────────────────────────────────────────────────────
struct Stat {
    uint16_t    type;
    uint32_t    dev;
    Qid         qid;
    uint32_t    mode;
    uint32_t    atime;
    uint32_t    mtime;
    uint64_t    length;
    std::string name;
    std::string uid;
    std::string gid;
    std::string muid;
};

// ─────────────────────────────────────────────────────────────────────────────
// 9P Message — Wire-format message
// ─────────────────────────────────────────────────────────────────────────────
struct Message {
    uint32_t    size;
    MsgType     type;
    uint16_t    tag;
    // Payload fields (union-like, used depending on type)
    uint32_t    fid;
    uint32_t    afid;
    uint32_t    newfid;
    uint64_t    offset;
    uint32_t    count;
    uint32_t    msize;
    std::string version_str;
    std::string uname;
    std::string aname;
    std::string ename;  // error string
    std::vector<std::string> wnames;
    std::vector<Qid>        wqids;
    Qid         qid;
    uint32_t    iounit;
    std::vector<uint8_t> data;

    Message() : size(0), type(MsgType::Tversion), tag(0),
                fid(0), afid(0), newfid(0), offset(0),
                count(0), msize(8192), iounit(0) {}

    // Serialize to wire format (little-endian)
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buf;
        auto put8  = [&](uint8_t v)  { buf.push_back(v); };
        auto put16 = [&](uint16_t v) { put8(v & 0xFF); put8((v >> 8) & 0xFF); };
        auto put32 = [&](uint32_t v) { put16(v & 0xFFFF); put16((v >> 16) & 0xFFFF); };
        auto put64 = [&](uint64_t v) { put32(v & 0xFFFFFFFF); put32((v >> 32) & 0xFFFFFFFF); };
        auto putstr = [&](const std::string& s) {
            put16(static_cast<uint16_t>(s.size()));
            for (char c : s) put8(static_cast<uint8_t>(c));
        };
        auto putqid = [&](const Qid& q) { put8(q.type); put32(q.version); put64(q.path); };

        // Reserve 4 bytes for size
        buf.resize(4);
        put8(static_cast<uint8_t>(type));
        put16(tag);

        switch (type) {
            case MsgType::Tversion: put32(msize); putstr(version_str); break;
            case MsgType::Rversion: put32(msize); putstr(version_str); break;
            case MsgType::Tattach:  put32(fid); put32(afid); putstr(uname); putstr(aname); break;
            case MsgType::Rattach:  putqid(qid); break;
            case MsgType::Rerror:   putstr(ename); break;
            case MsgType::Twalk:
                put32(fid); put32(newfid);
                put16(static_cast<uint16_t>(wnames.size()));
                for (auto& w : wnames) putstr(w);
                break;
            case MsgType::Rwalk:
                put16(static_cast<uint16_t>(wqids.size()));
                for (auto& q : wqids) putqid(q);
                break;
            case MsgType::Tread:  put32(fid); put64(offset); put32(count); break;
            case MsgType::Rread:
                put32(static_cast<uint32_t>(data.size()));
                for (auto b : data) put8(b);
                break;
            case MsgType::Twrite:
                put32(fid); put64(offset);
                put32(static_cast<uint32_t>(data.size()));
                for (auto b : data) put8(b);
                break;
            case MsgType::Rwrite: put32(count); break;
            case MsgType::Topen:  put32(fid); put8(0); break;
            case MsgType::Ropen:  putqid(qid); put32(iounit); break;
            case MsgType::Tclunk: put32(fid); break;
            case MsgType::Rclunk: break;
            default: break;
        }

        // Patch size
        uint32_t total = static_cast<uint32_t>(buf.size());
        buf[0] = total & 0xFF;
        buf[1] = (total >> 8) & 0xFF;
        buf[2] = (total >> 16) & 0xFF;
        buf[3] = (total >> 24) & 0xFF;
        return buf;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// CogFS Node — Virtual filesystem node backed by AtomSpace
// ─────────────────────────────────────────────────────────────────────────────
struct FsNode {
    std::string name;
    bool        is_dir;
    Qid         qid;
    Handle      atom_handle; // Link to AtomSpace atom
    std::vector<std::shared_ptr<FsNode>> children;
    std::string content;     // For file nodes

    FsNode() : is_dir(false), atom_handle(UNDEFINED_HANDLE) {
        qid.type = Qid::QTFILE;
        qid.version = 0;
        qid.path = 0;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// CogFS — Virtual filesystem exposing AtomSpace as 9P namespace
// ─────────────────────────────────────────────────────────────────────────────
class CogFS {
public:
    explicit CogFS(AtomSpace& as) : as_(as), next_path_(1) {
        root_ = std::make_shared<FsNode>();
        root_->name = "/";
        root_->is_dir = true;
        root_->qid.type = Qid::QTDIR;
        root_->qid.path = next_path_++;
    }

    // Rebuild filesystem tree from AtomSpace
    void sync() {
        root_->children.clear();
        // Create /nodes directory
        auto nodes_dir = make_dir("nodes");
        // Create /links directory
        auto links_dir = make_dir("links");
        // Create /types directory
        auto types_dir = make_dir("types");

        as_.foreach_atom([&](const Atom& a) {
            auto node = std::make_shared<FsNode>();
            node->name = std::to_string(a.handle);
            node->is_dir = false;
            node->qid.type = Qid::QTFILE;
            node->qid.path = next_path_++;
            node->atom_handle = a.handle;
            node->content = a.to_short_string();

            if (a.is_node()) {
                nodes_dir->children.push_back(node);
            } else {
                links_dir->children.push_back(node);
            }
        });

        root_->children.push_back(nodes_dir);
        root_->children.push_back(links_dir);
        root_->children.push_back(types_dir);

        // Populate types directory
        for (uint16_t t = 0; t < static_cast<uint16_t>(AtomType::ATOM_TYPE_COUNT); ++t) {
            auto handles = as_.get_by_type(static_cast<AtomType>(t));
            if (!handles.empty()) {
                auto tdir = std::make_shared<FsNode>();
                tdir->name = atom_type_name(static_cast<AtomType>(t));
                tdir->is_dir = true;
                tdir->qid.type = Qid::QTDIR;
                tdir->qid.path = next_path_++;
                for (auto h : handles) {
                    auto f = std::make_shared<FsNode>();
                    f->name = std::to_string(h);
                    f->is_dir = false;
                    f->qid.type = Qid::QTFILE;
                    f->qid.path = next_path_++;
                    f->atom_handle = h;
                    const Atom* atom = as_.get_atom(h);
                    if (atom) f->content = atom->to_short_string();
                    tdir->children.push_back(f);
                }
                types_dir->children.push_back(tdir);
            }
        }
    }

    // Walk a path from root
    std::shared_ptr<FsNode> walk(const std::vector<std::string>& path) const {
        auto current = root_;
        for (auto& name : path) {
            if (!current->is_dir) return nullptr;
            bool found = false;
            for (auto& child : current->children) {
                if (child->name == name) {
                    current = child;
                    found = true;
                    break;
                }
            }
            if (!found) return nullptr;
        }
        return current;
    }

    // Read file content
    std::string read(const std::vector<std::string>& path) const {
        auto node = walk(path);
        if (!node || node->is_dir) return "";
        return node->content;
    }

    // List directory
    std::vector<std::string> ls(const std::vector<std::string>& path) const {
        auto node = walk(path);
        if (!node || !node->is_dir) return {};
        std::vector<std::string> names;
        for (auto& child : node->children) {
            names.push_back(child->name + (child->is_dir ? "/" : ""));
        }
        return names;
    }

    // Write: create or update an atom via filesystem
    bool write(const std::vector<std::string>& path, const std::string& data) {
        if (path.size() < 2) return false;
        // Writing to /nodes/<name> creates a ConceptNode
        if (path[0] == "nodes") {
            as_.add_node(AtomType::CONCEPT_NODE, path[1],
                        TruthValue(1.0f, 0.9f));
            sync();
            return true;
        }
        return false;
    }

    const std::shared_ptr<FsNode>& root() const { return root_; }

private:
    AtomSpace& as_;
    std::shared_ptr<FsNode> root_;
    uint64_t next_path_;

    std::shared_ptr<FsNode> make_dir(const std::string& name) {
        auto d = std::make_shared<FsNode>();
        d->name = name;
        d->is_dir = true;
        d->qid.type = Qid::QTDIR;
        d->qid.path = next_path_++;
        return d;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// MachSpace — Memory-mapped AtomSpace region abstraction
// ─────────────────────────────────────────────────────────────────────────────
struct MachRegion {
    uint64_t base_addr;
    uint64_t size;
    uint32_t prot;  // PROT_READ=1, PROT_WRITE=2, PROT_EXEC=4
    std::string name;
    Handle atom_handle;

    static constexpr uint32_t PROT_READ  = 1;
    static constexpr uint32_t PROT_WRITE = 2;
    static constexpr uint32_t PROT_EXEC  = 4;
};

class MachSpace {
public:
    MachSpace() : next_addr_(0x1000000) {}

    // Map an atom into virtual address space
    MachRegion map_atom(Handle h, uint64_t size, uint32_t prot = MachRegion::PROT_READ) {
        MachRegion r;
        r.base_addr = next_addr_;
        r.size = size;
        r.prot = prot;
        r.name = "atom:" + std::to_string(h);
        r.atom_handle = h;
        next_addr_ += (size + 0xFFF) & ~0xFFFULL; // Page-align
        regions_[h] = r;
        return r;
    }

    // Lookup region by atom handle
    const MachRegion* find(Handle h) const {
        auto it = regions_.find(h);
        return (it != regions_.end()) ? &it->second : nullptr;
    }

    // Unmap
    bool unmap(Handle h) {
        return regions_.erase(h) > 0;
    }

    // List all mapped regions
    std::vector<MachRegion> list() const {
        std::vector<MachRegion> result;
        for (auto& kv : regions_) result.push_back(kv.second);
        return result;
    }

private:
    uint64_t next_addr_;
    std::unordered_map<Handle, MachRegion> regions_;
};

// ─────────────────────────────────────────────────────────────────────────────
// CogGrip — Cognitive grip metric for Plan 9 namespace coherence
// ─────────────────────────────────────────────────────────────────────────────
struct CogGrip {
    float relevance;     // How relevant is the namespace to the task
    float coherence;     // Internal consistency of the namespace
    float participation; // How actively used

    CogGrip() : relevance(0), coherence(0), participation(0) {}
    CogGrip(float r, float c, float p) : relevance(r), coherence(c), participation(p) {}

    float score() const { return (relevance + coherence + participation) / 3.0f; }
};

}} // namespace cog::plan9

#endif // COG_PLAN9_HPP
