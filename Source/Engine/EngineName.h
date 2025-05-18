#pragma once

#include <cstdint>
#include <functional>
#include <string>

/**
 * Using this instead of strins
 * I can hash them on start and easily compare ints instead of using strings in maps
 */

struct EngineName
{
    EngineName() = default;
    EngineName(uint32_t inHash)
        : hash(inHash)
    {
    }

    EngineName(const std::string &literalName)
    {
#if WITH_EDITOR
        editorName = literalName;
#endif

        HashName(literalName);
    }

    EngineName &operator=(uint32_t inHash)
    {
        hash = inHash;
        return *this;
    }

    EngineName &operator=(const std::string &literalName)
    {
        HashName(literalName);
        return *this;
    }

    inline bool operator==(uint32_t otherHash) const
    {
        return hash == otherHash;
    }

    inline bool operator!=(uint32_t otherHash) const
    {
        return hash != otherHash;
    }

    inline bool operator==(const EngineName &other) const
    {
        return hash == other.hash;
    }

    inline bool operator!=(const EngineName &other) const
    {
        return !(other.hash == hash);
    }

    void Clear()
    {
        hash = 0;
    }

    bool IsValid() const { return hash == 0; }

#if WITH_EDITOR
    // Only need this in the editor, the actual game should never use this
    int32_t editorNameSize = 0;
    std::string editorName;
#endif

    uint32_t hash = 0;

private:
    void HashName(const std::string &literalName)
    {
        std::hash<std::string> hasher;
        hash = hasher(literalName);
    }

    friend struct std::hash<EngineName>;
};

template <>
struct std::hash<EngineName>
{
    size_t operator()(const EngineName &s) const noexcept
    {
        return s.hash;
    }
};
