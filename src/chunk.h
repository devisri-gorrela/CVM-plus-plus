/**
 * @file chunk.h
 * @brief Bytecode container with RLE-compressed source line mapping.
 *
 * The Chunk struct stores compiled bytecode as a dense vector of bytes along
 * with a Run-Length Encoded (RLE) line table that maps bytecode offsets back
 * to source line numbers. This design dramatically reduces memory usage compared
 * to storing a line number per instruction, while still enabling accurate
 * source-level error reporting at runtime via binary-search lookups.
 */

#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>
#include "error.h"

struct LineEntry {
    size_t offset;
    int line;
};

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<LineEntry> lines;
    
    inline void reserve(size_t capacity) {
        code.reserve(capacity);
        // lines will be much smaller with RLE, so we don't reserve as much
        lines.reserve(capacity / 4);
    }

    inline void write(uint8_t byte, int line) {
        code.push_back(byte);
        if (lines.empty() || lines.back().line != line) {
            lines.push_back({code.size() - 1, line});
        }
    }

    inline void writeByte(uint8_t byte, int line) {
        write(byte, line);
    }
    
    inline void writeInt(int32_t value, int line) {
        size_t startOffset = code.size();
        code.push_back((value >> 24) & 0xFF);
        code.push_back((value >> 16) & 0xFF);
        code.push_back((value >> 8) & 0xFF);
        code.push_back(value & 0xFF);
        
        if (lines.empty() || lines.back().line != line) {
            lines.push_back({startOffset, line});
        }
    }

    inline void writeShort(uint16_t value, int line) {
        size_t startOffset = code.size();
        code.push_back((value >> 8) & 0xFF);
        code.push_back(value & 0xFF);
        
        if (lines.empty() || lines.back().line != line) {
            lines.push_back({startOffset, line});
        }
    }

    [[nodiscard]] int getLine(size_t offset) const {
        if (lines.empty()) return -1;

        // Binary search to find the entry with offset > given offset
        auto it = std::upper_bound(lines.begin(), lines.end(), offset,
            [](size_t val, const LineEntry& entry) {
                return val < entry.offset;
            });

        // The correct line is the one before the first entry that is greater than offset
        if (it == lines.begin()) return lines.front().line;
        return std::prev(it)->line;
    }

    [[nodiscard]] int32_t readInt(size_t offset) const {
        if (code.size() < 4 || offset > code.size() - 4) {
            throw RuntimeError("Bytecode truncated: attempted read past end");
        }
        return static_cast<int32_t>(
               (static_cast<uint32_t>(code[offset]) << 24) |
               (static_cast<uint32_t>(code[offset + 1]) << 16) |
               (static_cast<uint32_t>(code[offset + 2]) << 8) |
               (static_cast<uint32_t>(code[offset + 3]))
        );
    }

    [[nodiscard]] uint16_t readShort(size_t offset) const {
        if (code.size() < 2 || offset > code.size() - 2) {
            throw RuntimeError("Bytecode truncated: attempted read past end");
        }
        return static_cast<uint16_t>(
               (static_cast<uint16_t>(code[offset]) << 8) |
               (static_cast<uint16_t>(code[offset + 1]))
        );
    }
};
