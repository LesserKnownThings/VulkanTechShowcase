#include "SkeletonViewerWindow.h"
#include "AssetManager/AssetManager.h"
#include "AssetManager/Animation/Skeleton.h"
#include "AssetManager/Animation/BoneData.h"

#include <imgui/imgui.h>
#include <string>

void SkeletonViewerWindow::Initialize()
{
	uint32_t handles[1];
	AssetManager::Get().QueryAssets(handles, "Animations\\Dance");
	handle = handles[0];

    isShowing = true;
}

void SkeletonViewerWindow::UnInitialize()
{
}

void SkeletonViewerWindow::Draw()
{
	if (ImGui::Begin("SkeletonViewer", &isShowing))
	{
		/*Skeleton* skeleton = AssetManager::Get().LoadAsset<Skeleton>(handle);
		const SkeletonData& data = skeleton->GetSkeletonData();
        DrawBoneTree(data.rootBone);*/
	}

	ImGui::End();
}

void SkeletonViewerWindow::DrawBoneTree(const BoneNode& node)
{
    ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_OpenOnArrow;

    // If no children, mark as leaf
    if (node.children.empty()) 
    {
        baseFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        ImGui::TreeNodeEx(node.name.editorName.c_str(), baseFlags);
    }
    else 
    {
        bool opened = ImGui::TreeNodeEx(node.name.editorName.c_str(), baseFlags);
        if (opened) {
            for (const BoneNode& child : node.children)
            {
                DrawBoneTree(child);
            }
            ImGui::TreePop();
        }
    }
}
