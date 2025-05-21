#pragma once

#include "Asset.h"
#include "AssetPath.h"

#include <cassert>
#include <cstdint>
#include <iostream>

constexpr uint8_t FLAG_IS_COPY = 1 << 0;
constexpr uint8_t FLAG_IS_VALID = 1 << 1;

struct LazyAssetPtr
{
private:
	struct RefCounter
	{
		RefCounter(int32_t inCounter)
			: counter(inCounter) {}

		void Increment()
		{
			++counter;
		}

		void Decrement()
		{
			--counter;
		}

		int32_t counter = 0;
	};

public:
	~LazyAssetPtr();

	LazyAssetPtr()
		: flags(0)
	{}

	LazyAssetPtr(const AssetPath& inAssetPath)
		: path(inAssetPath), refCounter(new RefCounter(0)), flags(FLAG_IS_VALID)
	{}

	LazyAssetPtr(LazyAssetPtr&& other) noexcept
		: path(std::move(other.path)), refCounter(other.refCounter), ptr(other.ptr), flags(FLAG_IS_VALID)
	{
		refCounter->Increment();
		other.refCounter = nullptr;
		other.ptr = nullptr;
	}

	LazyAssetPtr& operator=(LazyAssetPtr&& other)
	{
		if (this != &other)
		{
			flags |= FLAG_IS_VALID;
			path = other.path;
			refCounter = other.refCounter;
			ptr = other.ptr;
			refCounter->Increment();
		}
		return *this;
	}

	template <typename T>
	T* Get() const;

	bool IsValid() const;
	const AssetPath& GetPath() const { return path; }

	void SetPath(const AssetPath& inPath);

private:
	LazyAssetPtr(const LazyAssetPtr& other)
		: ptr(other.ptr), refCounter(other.refCounter), path(other.path) {}

	LazyAssetPtr& operator=(const LazyAssetPtr& other)
	{
		if (this != &other)
		{
			flags = other.flags;
			flags |= FLAG_IS_COPY;
			path = other.path;
			refCounter = other.refCounter;
			ptr = other.ptr;
		}
		return *this;
	}

	void DecrementAssetCounter();

	AssetPath path;
	uint8_t flags = 0;

	RefCounter* refCounter = nullptr;
	mutable Asset* ptr = nullptr;

	// Only the asset manager should call copy
	friend class AssetManager;
};

template <typename T>
inline T* LazyAssetPtr::Get() const
{
	if (ptr == nullptr)
	{
		ptr = new T();
		assert(("Failed to load asset, provided an invalid path or extension!", ptr->LoadAsset(path.fullPath)));
	}
	return reinterpret_cast<T*>(ptr);
}
