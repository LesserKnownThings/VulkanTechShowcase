#pragma once

#include <array>
#include <functional>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <string>

struct UniqueID
{
	friend class IDManager;

public:
	~UniqueID();

	std::array<uint8_t, 16> data;
	size_t hash = 0;

	std::string ToString() const;
	void FromString(const std::string& guid_string);

	bool operator==(const UniqueID& other) const
	{
		return data == other.data;
	}

	bool operator<(const UniqueID& other) const
	{
		return data < other.data;
	}

private:
#ifdef _WIN32
	static UniqueID GenerateGUID();
#elif __linux__
	static UniqueID GenerateGUID();
#else
#error Platform not supported
#endif
};

template <>
struct std::hash<UniqueID>
{
	size_t operator()(const UniqueID& ID) const
	{
		return ID.hash;
	}
};

class IDManager
{
	friend struct UniqueID;

public:
	static UniqueID GenerateGUID();

	static IDManager& Get();

private:
	UniqueID InternalGenerateGUID();
	void RemoveGUID(size_t guid);
	bool AddGUID(const UniqueID& guid);
	bool GUIDExists(const UniqueID& guid);

	std::unordered_set<size_t> guids;

	static IDManager* instance;

	friend class Engine;
};