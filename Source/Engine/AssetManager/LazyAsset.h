#pragma once

#include "Asset.h"
#include "AssetPath.h"

#include <cassert>
#include <cstdint>
#include <iostream>

// This should only be used in the asset manager
struct LazyAsset
{
	~LazyAsset();

private:
	LazyAsset() = default;
	LazyAsset(const AssetPath& inPath)
		: path(inPath) {}

	Asset* asset = nullptr;
	AssetPath path;
	uint32_t counter = 0;

	template<typename T>
	T* Get();

	void Increment();
	void Decrement();

	friend class AssetManager;
};

template<typename T>
inline T* LazyAsset::Get()
{
	if (asset == nullptr)
	{
		asset = new T();
		asset->LoadAsset(path.fullPath);
	}

	return reinterpret_cast<T*>(asset);
}
