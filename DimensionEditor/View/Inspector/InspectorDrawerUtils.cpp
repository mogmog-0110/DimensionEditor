#include "InspectorDrawerUtils.hpp"
#include "../../SchemaManager.hpp"
#include "../../imgui-s3d-wrapper/imgui/DearImGuiAddon.hpp"

// JSONの値を編集するためのUIを描画する、再帰的なヘルパー関数
void DrawJsonValueEditor(const String& label, JSON& jsonValue, const std::shared_ptr<Schema>& childSchemaHint)
{
	ImGui::PushID(label.narrow().c_str());

	switch (jsonValue.getType())
	{
	case JSONValueType::String:
	{
		char buffer[1024];
		strcpy_s(buffer, jsonValue.getOr<String>(U"").toUTF8().c_str());
		if (ImGui::InputText(label.toUTF8().c_str(), buffer, std::size(buffer)))
		{
			jsonValue = Unicode::FromUTF8(buffer);
		}
		break;
	}
	case JSONValueType::Number:
	{
		double value = jsonValue.getOr<double>(0.0);
		if (ImGui::InputDouble(label.toUTF8().c_str(), &value))
		{
			jsonValue = value;
		}
		break;
	}
	case JSONValueType::Bool:
	{
		bool value = jsonValue.getOr<bool>(false);
		if (ImGui::Checkbox(label.toUTF8().c_str(), &value))
		{
			jsonValue = value;
		}
		break;
	}
	case JSONValueType::Array:
	{
		if (ImGui::TreeNode(label.toUTF8().c_str()))
		{
			int32 removeIndex = -1;

			for (auto&& [i, element] : IndexedRef(jsonValue.arrayView()))
			{
				String elementLabel = label + U"[" + ToString(i) + U"]";

				ImGui::PushID(static_cast<int>(i));

				if (ImGui::Button("-")) { removeIndex = static_cast<int32>(i); }
				ImGui::SameLine();

				if (ImGui::TreeNode(elementLabel.toUTF8().c_str()))
				{
					if (childSchemaHint && element.isObject())
					{
						ImGui::Indent();
						for (const auto& childPair : *childSchemaHint)
						{
							const String& childKey = childPair.first;
							const SchemaProperty& childProp = childPair.second;
							if (element.hasElement(childKey))
							{
								JSON valueCopy = element[childKey];
								DrawJsonValueEditor(childProp.description, valueCopy, childProp.childSchema);
								element[childKey] = valueCopy;
							}
						}
						ImGui::Unindent();
					}
					else
					{
						DrawJsonValueEditor(U"Value", element, nullptr);
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			if (removeIndex != -1)
			{
				jsonValue.erase(removeIndex);
			}

			if (ImGui::Button("+ Add"))
			{
				if (childSchemaHint)
				{
					JSON newObject;
					// 子スキーマを元に、正しい型のプロパティを追加
					for (const auto& childPair : *childSchemaHint)
					{
						const String& key = childPair.first;
						const SchemaProperty& prop = childPair.second;
						switch (prop.type)
						{
						case JSONValueType::String: newObject[key] = U""; break;
						case JSONValueType::Number: newObject[key] = 0; break;
						case JSONValueType::Bool:   newObject[key] = false; break;
						case JSONValueType::Array:  newObject[key] = Array<JSON>{}; break;
						case JSONValueType::Object: newObject[key] = JSON{}; break;
						default: newObject[key] = JSON(); break;
						}
					}
					jsonValue.push_back(newObject);
				}
				else
				{
					jsonValue.push_back(JSON());
				}
			}
			ImGui::TreePop();
		}
		break;
	}
	case JSONValueType::Object:
	{
		if (ImGui::TreeNode(label.toUTF8().c_str()))
		{
			Array<String> keys;
			for (const auto& pair : jsonValue)
			{
				keys.push_back(pair.key);
			}

			// 取得したキーのリストを使ってループ
			for (const auto& key : keys)
			{
				JSON valueCopy = jsonValue[key];

				if (childSchemaHint && (childSchemaHint->find(key) != childSchemaHint->end()))
				{
					const auto& prop = childSchemaHint->at(key);
					DrawJsonValueEditor(prop.description, valueCopy, prop.childSchema);
				}
				else
				{
					DrawJsonValueEditor(key, valueCopy, nullptr);
				}

				jsonValue[key] = valueCopy;
			}
			ImGui::TreePop();
		}
		break;
	}
	default:
		ImGui::Text((label + U" (Unknown Type)").toUTF8().c_str());
		break;
	}

	ImGui::PopID();
}
