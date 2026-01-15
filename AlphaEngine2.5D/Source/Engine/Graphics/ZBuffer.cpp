#include "ZBuffer.hpp"
#include <algorithm>

namespace Engine {

    std::vector<float> ZBuffer::buffer;

    void ZBuffer::Resize(int width) {
        if (width < 0) width = 0;
        if (buffer.size() != (size_t)width) {
            buffer.resize(width);
        }
    }

    void ZBuffer::Clear(float defaultDepth) {
        std::fill(buffer.begin(), buffer.end(), defaultDepth);
    }

    void ZBuffer::SetDepth(int x, float depth) {
        if (x >= 0 && x < (int)buffer.size()) {
            buffer[x] = depth;
        }
    }

    float ZBuffer::GetDepth(int x) {
        if (x >= 0 && x < (int)buffer.size()) {
            return buffer[x];
        }
        return 1e30f;
    }

    int ZBuffer::GetWidth() {
        return (int)buffer.size();
    }

} // namespace Engine
