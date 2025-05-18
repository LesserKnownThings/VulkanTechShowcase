#include "LazyAssetPtr.h"

LazyAssetPtr::~LazyAssetPtr()
{
	DecrementAssetCounter();
}

void LazyAssetPtr::DecrementAssetCounter()
{
	if (refCounter && ptr)
	{
		refCounter->Decrement();

		if (refCounter->counter == 0)
		{
			ptr->UnloadAsset();
			ptr = nullptr;
			delete ptr;
		}
	}
}

void LazyAssetPtr::SetPath(const AssetPath& inPath)
{
	path = inPath;
}

bool LazyAssetPtr::IsValid() const
{
	return ptr != nullptr;
}
