#include "RoomConnectionsDrawer.hpp"
#include "../../SchemaManager.hpp"
#include "../EditorView.hpp"
#include "../../Controller/EditorController.hpp"

void RoomConnectionsDrawer::draw(JSON& jsonData, EditorView& editorView, EditorController& controller, DimensionModel&)
{
	if (not jsonData.hasElement(U"rooms") || not jsonData[U"rooms"].isObject())
	{
		ImGui::Text("Invalid format: 'rooms' object not found.");
		return;
	}

	// ヘッダー
	if (ImGui::CollapsingHeader("Rooms", ImGuiTreeNodeFlags_DefaultOpen))
	{
		String roomToDelete = U""; // 削除対象の部屋名を一時的に保持

		// 既存の部屋をリスト表示
		for (const auto& roomPair : jsonData[U"rooms"])
		{
			const String& roomName = roomPair.key;
			ImGui::PushID(roomName.toUTF8().c_str());

			// 部屋名を表示
			ImGui::BulletText(roomName.toUTF8().c_str());

			ImGui::SameLine(ImGui::GetWindowWidth() - 120); // ボタンを右端に寄せる

			// 編集ボタン
			if (ImGui::Button("Edit"))
			{
				editorView.m_editingRoomName = roomName;
				editorView.m_editingRoomDataCopy = roomPair.value;
				editorView.m_showRoomEditor = true;
			}
			ImGui::SameLine();

			// 削除ボタン
			if (ImGui::Button("Delete"))
			{
				// すぐには削除せず、ループの外で処理するために名前を記録
				roomToDelete = roomName;
			}

			ImGui::PopID();
		}

		// ループの外で安全に要素を削除
		if (not roomToDelete.isEmpty())
		{
			jsonData[U"rooms"].erase(roomToDelete);
		}

		ImGui::Separator();
	}

	// "Add Room"ボタン
	if (ImGui::Button("Add Room...", ImVec2(-1, 0))) // 横幅いっぱいに広げる
	{
		m_newRoomNameBuffer.clear();
		ImGui::OpenPopup("Add New Room");
	}

	// "Add Room"ポップアップの実装
	if (ImGui::BeginPopupModal("Add New Room", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputText("New Room Name", &m_newRoomNameBuffer);

		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			if (not m_newRoomNameBuffer.empty())
			{
				const String newRoomName = Unicode::FromUTF8(m_newRoomNameBuffer);

				// 新しい部屋のデフォルトデータをスキーマから生成
				JSON newRoomData;
				for (const auto& propPair : *g_RoomSchema)
				{
					const auto& key = propPair.first;
					const auto& prop = propPair.second;
					if (prop.isRequired) // 必須プロパティのみ初期化
					{
						switch (prop.type)
						{
						case JSONValueType::String: newRoomData[key] = U""; break;
						case JSONValueType::Object: newRoomData[key] = JSON(); break;
						case JSONValueType::Array: newRoomData[key] = Array<JSON>(); break;
						default: break;
						}
					}
				}
				// layoutオブジェクトの必須プロパティも初期化
				newRoomData[U"layout"][U"forcusable"] = Array<JSON>();
				newRoomData[U"layout"][U"interactable"] = Array<JSON>(); 

				jsonData[U"rooms"][newRoomName] = newRoomData;
				controller.addNewRoom(newRoomName);
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
