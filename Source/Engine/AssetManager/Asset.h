#pragma once

#include "Object.h"

#include <string>
#include <vector>

class Asset : public Object
{
public:
    Asset() = default;
    
    virtual bool LoadAsset(const std::string &path) = 0;
    virtual void UnloadAsset() = 0;
};