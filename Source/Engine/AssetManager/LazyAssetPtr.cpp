#include "LazyAssetPtr.h"

LazyAssetPtr::~LazyAssetPtr()
{
	DecrementAssetCounter();
}

void LazyAssetPtr::DecrementAssetCounter()
{
	if (!(flags & FLAG_IS_COPY) && refCounter)
	{
		refCounter->Decrement();

		if (refCounter->counter == 0 && ptr != nullptr)
		{
			ptr->UnloadAsset();
			delete ptr;
			ptr = nullptr;
		}
	}
}

void LazyAssetPtr::SetPath(const AssetPath& inPath)
{
	path = inPath;
}

bool LazyAssetPtr::IsValid() const
{
	return flags & FLAG_IS_VALID;
}
