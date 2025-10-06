#include "SchemaDrivenDrawer.hpp"
#include "../../imgui-s3d-wrapper/imgui/DearImGuiAddon.hpp"
#include "InspectorDrawerUtils.hpp"
#include "../EditorView.hpp"
#include "../../Controller/EditorController.hpp"

namespace
{
	String JSONValueTypeToString(const JSONValueType type)
	{
		switch (type)
		{
		case JSONValueType::String: return U"String";
		case JSONValueType::Number: return U"Number";
		case JSONValueType::Bool:   return U"Bool";
		case JSONValueType::Array:  return U"Array";
		case JSONValueType::Object: return U"Object";
		case JSONValueType::Null:   return U"Null";
		default:                    return U"Unknown";
		}
	}
}

SchemaDrivenDrawer::SchemaDrivenDrawer(const Schema& schema)
	: m_schema(schema)
{
}

void SchemaDrivenDrawer::draw(JSON& jsonData, EditorView& editorView, EditorController& controller, DimensionModel&)
{
	for (const auto& schemaPair : m_schema)
	{
		const String& key = schemaPair.first;
		const SchemaProperty& prop = schemaPair.second;

		if (jsonData.hasElement(key))
		{
			if (key == U"initial_grid")
			{
				if (key == U"initial_grid")
				{
					if (ImGui::TreeNode(prop.description.toUTF8().c_str()))
					{
						if (not jsonData[key].isArray())
						{
							jsonData[key] = Array<JSON>();
						}

						int height = static_cast<int>(jsonData[key].size());
						int width = (height > 0 && jsonData[key][0].isArray()) ? static_cast<int>(jsonData[key][0].size()) : 0;

						int newWidth = width;
						int newHeight = height;

						ImGui::PushID("GridSize");
						ImGui::Text("Size:");
						ImGui::SameLine();
						ImGui::SetNextItemWidth(80);
						if (ImGui::InputInt("W", &newWidth)) { newWidth = Clamp(newWidth, 0, 50); }
						ImGui::SameLine();
						ImGui::SetNextItemWidth(80);
						if (ImGui::InputInt("H", &newHeight)) { newHeight = Clamp(newHeight, 0, 50); }
						ImGui::PopID();

						if (newWidth != width || newHeight != height)
						{
							Array<Array<int>> newGrid(newHeight, Array<int>(newWidth, 0));
							for (int y = 0; y < Min(height, newHeight); ++y)
							{
								for (int x = 0; x < Min(width, newWidth); ++x)
								{
									newGrid[y][x] = jsonData[key][y][x].getOpt<int>().value_or(0);
								}
							}
							jsonData[key] = newGrid;
						}

						ImGui::Separator();
						for (int y = 0; y < newHeight; ++y)
						{
							ImGui::PushID(y);
							for (int x = 0; x < newWidth; ++x)
							{
								bool isChecked = (jsonData[key][y][x].getOpt<int>().value_or(0) == 1);
								std::string cellLabel = "##cell" + ToString(x).narrow();
								if (ImGui::Checkbox(cellLabel.c_str(), &isChecked))
								{
									jsonData[key][y][x] = isChecked ? 1 : 0;
								}
								if (x < newWidth - 1)
								{
									ImGui::SameLine();
								}
							}
							ImGui::PopID();
						}
						ImGui::TreePop();
					}
				}
			}
			else
			{
				// それ以外のプロパティは、従来通りの汎用エディタを呼び出す
				JSON valueCopy = jsonData[key];
				DrawJsonValueEditor(prop.description, valueCopy, prop.childSchema);
				jsonData[key] = valueCopy;
			}
		}
		else if (prop.isRequired)
		{
			const String errorMsg = (prop.description + U" [必須プロパティがありません！]");
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), errorMsg.toUTF8().c_str());
		}
	}
}
