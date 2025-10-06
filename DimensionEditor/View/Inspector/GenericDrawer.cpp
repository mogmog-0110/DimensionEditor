#include "GenericDrawer.hpp"
#include "../../imgui-s3d-wrapper/imgui/DearImGuiAddon.hpp"
#include "InspectorDrawerUtils.hpp"
#include "../EditorView.hpp"

void GenericDrawer::draw(JSON& jsonData, EditorView&, EditorController&, DimensionModel&)
{
	if (ImGui::CollapsingHeader("Generic Properties", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (jsonData.isObject())
		{
			Array<String> keys;
			for (const auto& pair : jsonData)
			{
				keys.push_back(pair.key);
			}

			for (const auto& key : keys)
			{
				JSON valueCopy = jsonData[key];
				DrawJsonValueEditor(key, valueCopy, nullptr);
				jsonData[key] = valueCopy;
			}
		}
	}
}
