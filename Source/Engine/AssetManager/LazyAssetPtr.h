#pragma once

#include "Asset.h"
#include "AssetPath.h"

#include <cassert>
#include <iostream>

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

	LazyAssetPtr() = default;

	LazyAssetPtr(const AssetPath& inAssetPath)
		: path(inAssetPath), refCounter(new RefCounter(0)) {}

	LazyAssetPtr(LazyAssetPtr&& other) noexcept
		: path(std::move(other.path)), refCounter(std::move(other.refCounter)), ptr(other.ptr)
	{
		other.refCounter = nullptr;
	}

	LazyAssetPtr& operator=(LazyAssetPtr&& other) noexcept
	{
		if (&other != this)
		{
			// We're assigning an asset to an existing ptr so we need to release it first
			if (ptr && refCounter)
			{
				DecrementAssetCounter();
			}

			ptr = std::move(other.ptr);
			other.ptr = nullptr;

			path = std::move(other.path);
			other.path = {};

			refCounter = std::move(other.refCounter);
			other.refCounter = nullptr;
		}
		return *this;
	}

	template <typename T>
	LazyAssetPtr& StrongRef();

	template <typename T>
	T* Get() const;

	bool IsValid() const;
	const AssetPath& GetPath() const { return path; }

	void SetPath(const AssetPath& inPath);

private:
	// Copy construct
	LazyAssetPtr(const LazyAssetPtr& other)
		: ptr(other.ptr), refCounter(other.refCounter), path(other.path) {}

	LazyAssetPtr& operator=(const LazyAssetPtr& other) = delete;
	LazyAssetPtr& operator=(LazyAssetPtr& other) = delete;

	template <typename T>
	void LoadAsset();

	void DecrementAssetCounter();

	AssetPath path;

	RefCounter* refCounter = nullptr;
	Asset* ptr = nullptr;
};

template <typename T>
inline LazyAssetPtr& LazyAssetPtr::StrongRef()
{
	refCounter->Increment();
	if (ptr == nullptr)
	{
		LoadAsset<T>();
	}

	LazyAssetPtr* strongAsset = new LazyAssetPtr(*this);
	return *strongAsset;
}

template <typename T>
inline T* LazyAssetPtr::Get() const
{
	return reinterpret_cast<T*>(ptr);
}

template <typename T>
inline void LazyAssetPtr::LoadAsset()
{
	ptr = new T();
	assert(("Failed to load asset, provided an invalid path or extension!", ptr->LoadAsset(path.fullPath)));
}
