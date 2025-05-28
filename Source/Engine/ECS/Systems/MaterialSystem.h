#pragma once

#include "Rendering/AbstractData.h"
#include "Rendering/Material/Material.h"

#include <cstdint>
#include <optional>
#include <unordered_map>

struct EngineName;
struct Material;

class MaterialSystem
{
public:
	void ReleaseResources();

	Material CreatePBRMaterial(std::optional<uint32_t> albedo = std::nullopt, bool makeUnique = false);

	bool TryGetMaterialInstance(uint32_t handle, MaterialInstance& outInstance) const;

	void SetTextures(uint32_t handle, const std::vector<MaterialDescriptorBindingResource>& resources);

private:
	std::unordered_map<uint32_t, MaterialInstance> materialInstances;
	std::unordered_map<MaterialInstanceKey, uint32_t> materialHandles;
	uint32_t materialHandleTracker = 0;
};