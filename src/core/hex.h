#ifndef CORE_HEX_H
#define CORE_HEX_H

#include <cstddef>
#include <functional>

struct Hex {
    int x = 0, y = 0, z = 0;
};

inline bool operator==(const Hex& a, const Hex& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline bool operator!=(const Hex& a, const Hex& b) {
    return !(a == b);
}

inline bool isValidHex(const Hex& h) {
    return h.x + h.y + h.z == 0;
}

inline size_t qHash(const Hex& h, size_t seed = 0) {
    size_t out = seed;
    out ^= static_cast<size_t>(h.x) * 73856093u;
    out ^= static_cast<size_t>(h.y) * 19349663u;
    out ^= static_cast<size_t>(h.z) * 83492791u;
    return out;
}

namespace std {
template <>
struct hash<Hex> {
    size_t operator()(const Hex& h) const noexcept {
        return qHash(h);
    }
};
}

#endif // CORE_HEX_H
