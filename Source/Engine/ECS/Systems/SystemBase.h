#pragma once

#include <cstdint>
#include <vector>

struct Entity;

class SystemBase
{
public:
    virtual void Initialize();
    virtual void UnInitialize() = 0;

protected:
    virtual void Allocate(int32_t size) = 0;
    virtual void HandleEntityRemoved(const Entity &entity) = 0;
    virtual void DestroyComponent(uint32_t componentID) = 0;

    std::vector<uint32_t> pendingDeletion;

private:
    void GCPass();

    friend class Engine;
};