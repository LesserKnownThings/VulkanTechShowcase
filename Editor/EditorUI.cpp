#include "EditorUI.h"
#include "Rendering/Vulkan/VkContext.h"
#include "Rendering/Vulkan/VulkanRendering.h"
#include "TaskManager.h"
#include "Windows/MainWindow.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl3.h>
#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#include <imgui/imgui_impl_vulkan.h>

#pragma once

static ImGui_ImplVulkanH_Window mainWindow;

bool EditorUI::Initialize(RenderingInterface* renderingSystem)
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	io.DeltaTime = 0.05f;

	// Setup Dear ImGui style
	// ImGui::StyleColorsClassic();
	ImGui::StyleColorsLight();

	if (VulkanRendering* vk = reinterpret_cast<VulkanRendering*>(renderingSystem))
	{
		const VkContext& context = vk->GetContext();

		ImGui_ImplSDL3_InitForVulkan(vk->GetWindow());

		ImGui_ImplVulkan_LoadFunctions([](const char* name, void* user_data) {
			return vkGetInstanceProcAddr((VkInstance)user_data, name);
			}, context.instance);

		ImGui_ImplVulkan_InitInfo info{};
		info.Instance = context.instance;
		info.PhysicalDevice = context.physicalDevice;
		info.Device = context.device;
		info.QueueFamily = context.familyIndices.graphicsFamily.value();
		info.Queue = context.graphicsQueue;
		info.PipelineCache = VK_NULL_HANDLE;
		info.DescriptorPool = context.descriptorPool;
		info.RenderPass = context.renderPass;
		info.Subpass = 0;
		info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
		info.ImageCount = MAX_FRAMES_IN_FLIGHT;
		info.MSAASamples = context.msaaSamples;
		info.Allocator = VK_NULL_HANDLE;

		ImGui_ImplVulkan_Init(&info);

		mainWindow = new MainWindow();
		mainWindow->Initialize();

		return true;
	}

	return false;
}

void EditorUI::UnInitialize()
{
	mainWindow->UnInitialize();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

void EditorUI::DrawFrame(uintptr_t commandBuffer)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	mainWindow->Draw();

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawData, reinterpret_cast<VkCommandBuffer>(commandBuffer));

}
