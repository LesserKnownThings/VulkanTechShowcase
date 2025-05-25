#include "LazyAsset.h"

LazyAsset::~LazyAsset()
{
	if (asset != nullptr)
	{
		asset->UnloadAsset();
		delete asset;
		asset = nullptr;
	}
}

void LazyAsset::Increment()
{
	counter++;
}

void LazyAsset::Decrement()
{
	counter--;
	if (counter <= 0 && asset != nullptr)
	{
		asset->UnloadAsset();
		delete asset;
		asset = nullptr;
	}
}
