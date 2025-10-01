#include "EditorView.hpp"

#include <Siv3D.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "DearImGuiAddon.hpp"

#include "../Model/DimensionModel.hpp"
#include "../Controller/EditorController.hpp"


EditorView::EditorView()
{
	strcpy_s(m_newDimensionNameBuffer, "dimension_new");

	ImGui::CreateContext();
	auto& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

}

void EditorView::draw(DimensionModel& model, EditorController& controller)
{ 
	ImGui::DockSpaceOverViewport(ImGui::GetID("DockSpace"), ImGui::GetMainViewport());

	drawMenuBar(controller);

	if (m_shouldShowNewDimensionPopup)
	{
		ImGui::OpenPopup("Create New Dimension");
		m_shouldShowNewDimensionPopup = false;
	}
	if (ImGui::BeginPopupModal("Create New Dimension", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputText("Name", m_newDimensionNameBuffer, std::size(m_newDimensionNameBuffer));
		if (ImGui::Button("Create"))
		{
			controller.createNewDimension(Unicode::FromUTF8(m_newDimensionNameBuffer));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	drawHierarchyPanel(model, controller);
	drawCanvasPanel();
	drawInspectorPanel(controller);
	drawAssetsPanel();
}

void EditorView::drawMenuBar(EditorController& controller)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Dimension...")) { m_shouldShowNewDimensionPopup = true; }
			if (ImGui::MenuItem("Open Dimension...")) { controller.openDimension(); }
			if (ImGui::MenuItem("Save")) { controller.saveDimension(); }
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void EditorView::drawHierarchyPanel(DimensionModel& model, EditorController& controller)
{
	ImGui::Begin("Hierarchy");


	if (model.isDimensionLoaded())
	{
		const String& dimensionName = model.getDimensionName();

		if (dimensionName.isEmpty())
		{
			ImGui::Text("Error: Dimension name is empty.");
			ImGui::End();
			return;
		}

		if (not dimensionName.isEmpty() && ImGui::TreeNode(dimensionName.toUTF8().c_str()))
		{
			for (const auto& room : model.getRooms())
			{
				if (room.name.isEmpty()) continue;

				ImGuiTreeNodeFlags roomNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
				if (room.objects.isEmpty()) roomNodeFlags |= ImGuiTreeNodeFlags_Leaf;

				if (ImGui::TreeNodeEx(room.name.toUTF8().c_str(), roomNodeFlags))
				{
					for (const auto& object : room.objects)
					{
						if (object.fileName.isEmpty()) continue;

						ImGuiTreeNodeFlags objectNodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
						const FilePath objectPath = U"App/data/" + model.getDimensionName() + U"/" + room.name + U"/" + object.fileName;
						if (objectPath == controller.getSelectedPath()) objectNodeFlags |= ImGuiTreeNodeFlags_Selected;

						ImGui::TreeNodeEx(object.fileName.toUTF8().c_str(), objectNodeFlags);
						if (ImGui::IsItemClicked())
						{
							controller.setSelectedPath(objectPath);
						}
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
	{
		controller.setSelectedPath(U"");
	}
	ImGui::End();
}

void EditorView::drawCanvasPanel()
{
	ImGui::Begin("Canvas");
	ImGui::Text("Visual representation of the room here.");
	ImGui::End();
}

void EditorView::drawInspectorPanel(EditorController& controller)
{
	ImGui::Begin("Inspector");
	if (not controller.getSelectedPath().isEmpty())
	{
		ImGui::Text("Selected File:");
		ImGui::Text(controller.getSelectedPath().toUTF8().c_str());
	}
	else
	{
		ImGui::Text("No file selected.");
	}
	ImGui::End();
}

void EditorView::drawAssetsPanel()
{
	ImGui::Begin("Assets");
	ImGui::Text("Asset browser here.");
	ImGui::End();
}
