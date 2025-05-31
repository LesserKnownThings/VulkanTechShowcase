#include "MainWindow.h"
#include "SkeletonViewerWindow.h"

#include <imgui/imgui.h>

void MainWindow::Initialize()
{
	Window* skeletonViewer = new SkeletonViewerWindow();
	skeletonViewer->Initialize();
	windows.push_back(skeletonViewer);
}

void MainWindow::UnInitialize()
{

}

void MainWindow::Draw()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Set up a full-screen invisible window to contain the dockspace
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

	ImGui::Begin("MainDockSpace", nullptr,
		ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus);

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(3);

	ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	for (Window* window : windows)
	{
		window->Draw();
	}

	ImGui::End();

	PostDraw();
}

void MainWindow::PostDraw()
{
	for (auto it = windows.begin(); it != windows.end();)
	{
		Window* currentWindow = *it;
		if (!currentWindow->IsShowing())
		{
			it = windows.erase(it);
		}
		else
		{
			++it;
		}
	}
}
