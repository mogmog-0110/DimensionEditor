#include "EditorView.hpp"

#include <Siv3D.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui-s3d-wrapper/imgui/DearImGuiAddon.hpp"

#include "../Model/DimensionModel.hpp"
#include "../Controller/EditorController.hpp"

namespace s3d
{
	void Formatter(FormatData& formatData, const JSONItem& item)
	{
		formatData.string += U"JSONItem";
	}
}

// アクションタイプの定義を拡張
namespace
{
	const char* ACTION_TYPES[] = {
		"テキストを表示 (ShowText)",
		"アイテムを入手 (GiveItem)",
		"フラグを操作 (SetFlag)",
		"条件分岐 (Conditional)",
		"連続実行 (Sequence)",
		"ステップ実行 (MultiStep)",
		"次元を移動 (ChangeDimension)"
	};
	enum ActionTypeIndex {
		ActionType_ShowText = 0,
		ActionType_GiveItem,
		ActionType_SetFlag,
		ActionType_Conditional,
		ActionType_Sequence,
		ActionType_MultiStep,
		ActionType_ChangeDimension
	};
}

int EditorView::s_selectedActionTypeIndex = 0;
int EditorView::s_selectedConditionTypeIndex = 0;

JSON createActionTemplate(const String& type);

// Conditionのテンプレートを生成する関数
JSON createConditionTemplate(const String& type)
{
	if (type == U"HasItem")
	{
		return JSON{
			{U"type", U"HasItem"},
			{U"item", U"（アイテムID）"}
		};
	}
	if (type == U"IsFlagOn")
	{
		return JSON{
			{U"type", U"IsFlagOn"},
			{U"flag", U"（フラグ名）"},
			{U"scope", U"Dimension"}
		};
	}
	// デフォルト
	return JSON{ {U"type", U"HasItem"} };
}


// Actionのテンプレートを生成する関数
JSON createActionTemplate(const String& type)
{
	if (type == U"ShowText")
	{
		return JSON{
			{U"type", U"ShowText"},
			{U"file", U"example.txt"}
		};
	}
	if (type == U"GiveItem")
	{
		return JSON{
			{U"type", U"GiveItem"},
			{U"item", U"（アイテムID）"}
		};
	}
	if (type == U"SetFlag")
	{
		return JSON{
			{U"type", U"SetFlag"},
			{U"flag", U"（フラグ名）"},
			{U"value", true},
			{U"scope", U"Dimension"}
		};
	}
	if (type == U"Conditional")
	{
		return JSON{
			{U"type", U"Conditional"},
			{U"condition", createConditionTemplate(U"HasItem")},
			{U"success", createActionTemplate(U"ShowText")},
			{U"failure", createActionTemplate(U"ShowText")}
		};
	}
	if (type == U"Sequence")
	{
		return JSON{
			{U"type", U"Sequence"},
			{U"actions", Array<JSON>()} // 空の配列
		};
	}
	if (type == U"MultiStep")
	{
		return JSON{
			{U"type", U"MultiStep"},
			{U"id", U"（ユニークなID）"},
			{U"steps", Array<JSON>()}, // 空の配列
			{U"final_action", createActionTemplate(U"ShowText")}
		};
	}
	// デフォルト
	return createActionTemplate(U"ShowText");
}

String GridToString(const Point& p)
{
	if (p.x < 0 || p.y < 0) return U"";
	return U"{}{}"_fmt(static_cast<char32>(U'A' + p.y), p.x + 1);
}

String GridRectToString(const Rect& r)
{
	const String start = GridToString(r.pos);
	const String end = GridToString(r.br() - Point{ 1, 1 });

	if (start == end)
	{
		return start;
	}
	return U"{}-{}"_fmt(start, end);
}

EditorView::EditorView()
{
	// デフォルトの作成先パスを設定
	m_newDimensionPathBuffer = "";
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
	if (m_shouldOpenAddHotspotModal)
	{
		ImGui::OpenPopup("Add New Hotspot");
		m_shouldOpenAddHotspotModal = false;
	}

	drawRoomEditorWindow(controller);
	drawGridSelectorWindow(controller);
	drawAddTransitionWindow(controller);
	drawForcusableEditorWindow(controller);
	drawInteractableEditorWindow(controller);

	if (ImGui::BeginPopupModal("Create New Dimension", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputText("Name", &m_newDimensionNameBuffer);
		ImGui::InputText("Directory", &m_newDimensionPathBuffer, ImGuiInputTextFlags_ReadOnly);
		ImGui::SameLine();
		if (ImGui::Button("Browse..."))
		{
			if (const auto result = Dialog::SelectFolder(Unicode::FromUTF8(m_newDimensionPathBuffer))) {
				m_newDimensionPathBuffer = result.value().toUTF8();
			}
		}
		if (ImGui::Button("Create", ImVec2(120, 0))) {
			controller.createNewDimension(Unicode::FromUTF8(m_newDimensionNameBuffer), Unicode::FromUTF8(m_newDimensionPathBuffer));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	drawAddHotspotModal(controller);
	drawHierarchyPanel(model, controller);
	drawCanvasPanel(controller);
	drawInspectorPanel(controller);
}

void EditorView::openInteractableEditor(int index)
{
	if (index == -1) {
		m_isEditingInteractable = false;
		m_interactableDraftState = {};
	}
	else {
		m_isEditingInteractable = true;
		m_editingInteractableIndex = index;
		const auto& item = m_editingRoomDataCopy[U"interactables"][index];
		m_interactableDraftState.nameBuffer = item[U"name"].getOpt<String>().value_or(U"").toUTF8();
		if (item.hasElement(U"default_state")) {
			const auto& defaultState = item[U"default_state"];
			m_interactableDraftState.defaultStateDraft.assetBuffer = defaultState[U"asset"].getOpt<String>().value_or(U"").toUTF8();
			m_interactableDraftState.defaultStateDraft.gridPosBuffer = defaultState[U"grid_pos"].getOpt<String>().value_or(U"").toUTF8();
		}

		m_interactableDraftState.states.clear();
		if (item.hasElement(U"states"))
		{
			for (const auto& stateJson : item[U"states"].arrayView())
			{
				ConditionalStateDraft stateDraft;
				stateDraft.conditionFlagBuffer = stateJson[U"condition_flag"].getOpt<String>().value_or(U"").toUTF8();
				stateDraft.assetBuffer = stateJson[U"asset"].getOpt<String>().value_or(U"").toUTF8();
				stateDraft.gridPosBuffer = stateJson[U"grid_pos"].getOpt<String>().value_or(U"").toUTF8();
				m_interactableDraftState.states.push_back(stateDraft);
			}
		}

		if (item.hasElement(U"hotspot")) {
			const auto& hotspot = item[U"hotspot"];
			m_interactableDraftState.hotspotDraft.gridPosBuffer = hotspot[U"grid_pos"].getOpt<String>().value_or(U"").toUTF8();
			if (hotspot.hasElement(U"action")) {
				buildDraftFromActionJson(m_interactableDraftState.hotspotDraft.rootAction, hotspot[U"action"]);
			}
		}
	}
	m_showAddInteractableWindow = (index == -1);
	m_showEditInteractableWindow = (index != -1);
}


void EditorView::openGridSelector(std::string& targetBuffer)
{
	m_gridSelectorTargetBuffer = &targetBuffer;
	m_gridDragStartCell = { -1, -1 };
	m_gridSelectionRect = { -1, -1, -1, -1 };
	m_showGridSelector = true;
}

void EditorView::drawRoomEditorWindow(EditorController& controller)
{
	if (!m_showRoomEditor)
	{
		return;
	}

	String modalTitle = U"Edit Room: {}"_fmt(m_editingRoomName);
	if (ImGui::Begin(modalTitle.toUTF8().c_str(), &m_showRoomEditor))
	{
		//==============================================================================
		// タブUI
		//==============================================================================
		if (ImGui::BeginTabBar("RoomEditorTabs"))
		{
			//--------------------------------------------------------------------------
			// Generalタブ
			//--------------------------------------------------------------------------
			if (ImGui::BeginTabItem("General"))
			{
				if (m_editingRoomDataCopy.hasElement(U"background") && m_editingRoomDataCopy[U"background"].isString())
				{
					std::string bgBuffer = m_editingRoomDataCopy[U"background"].get<String>().toUTF8();
					if (ImGui::InputText("Background Asset", &bgBuffer, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						m_editingRoomDataCopy[U"background"] = Unicode::FromUTF8(bgBuffer);
					}
				}
				ImGui::EndTabItem();
			}

			//--------------------------------------------------------------------------
			// Transitionsタブ
			//--------------------------------------------------------------------------
			if (ImGui::BeginTabItem("Transitions"))
			{
				if (not m_editingRoomDataCopy.hasElement(U"transitions")) { m_editingRoomDataCopy[U"transitions"] = JSON(); }
				String transitionToDelete = U"";
				for (const auto& transPair : m_editingRoomDataCopy[U"transitions"])
				{
					const auto& key = transPair.key;
					const auto& value = transPair.value;
					ImGui::PushID(key.toUTF8().c_str());
					ImGui::Bullet(); ImGui::SameLine();
					if (value.isString())
					{
						ImGui::Text("%s -> %s", key.toUTF8().c_str(), value.get<String>().toUTF8().c_str());
					}
					else if (value.isObject())
					{
						// まず行き先は必ず表示
						ImGui::Text("%s -> %s", key.toUTF8().c_str(), value[U"to"].get<String>().toUTF8().c_str());

						// conditionキーが存在する場合のみ、条件を表示
						if (value.hasElement(U"condition"))
						{
							ImGui::SameLine();
							ImGui::Text(" (if %s is TRUE)", value[U"condition"].get<String>().toUTF8().c_str());
						}
						// grid_posキーが存在する場合のみ、位置を表示
						if (value.hasElement(U"grid_pos"))
						{
							ImGui::SameLine();
							ImGui::Text(" (at %s)", value[U"grid_pos"].get<String>().toUTF8().c_str());
						}
					}

					ImGui::SameLine(ImGui::GetWindowWidth() - 40);
					if (ImGui::SmallButton("X")) { transitionToDelete = key; }
					ImGui::PopID();
				}

				if (not transitionToDelete.isEmpty())
				{
					m_editingRoomDataCopy[U"transitions"].erase(transitionToDelete);
				}
				ImGui::Separator();
				if (ImGui::Button("Add Transition..."))
				{
					m_newTransitionTypeIndex = 0;
					m_newTransitionDirectionIndex = 0;
					m_showAddTransitionWindow = true;
				}
				ImGui::EndTabItem();
			}

			//--------------------------------------------------------------------------
			// Layout & Objectsタブ
			//--------------------------------------------------------------------------
			if (ImGui::BeginTabItem("Layout & Objects"))
			{
				// --- Forcusable Objects ---
				ImGui::Text("Forcusable Objects");
				ImGui::Separator();
				if (not m_editingRoomDataCopy.hasElement(U"forcusables")) { m_editingRoomDataCopy[U"forcusables"] = Array<JSON>(); }

				int focusableIndexToDelete = -1;
				for (size_t i = 0; i < m_editingRoomDataCopy[U"forcusables"].size(); ++i)
				{
					const auto& item = m_editingRoomDataCopy[U"forcusables"][i];
					const String name = item[U"name"].getOpt<String>().value_or(U"");
					ImGui::PushID(static_cast<int>(i) + 1000); // InteractableとIDが衝突しないようにオフセット
					ImGui::BulletText(name.toUTF8().c_str());
					ImGui::SameLine(ImGui::GetWindowWidth() - 120);
					if (ImGui::SmallButton("Edit")) { openForcusableEditor(static_cast<int>(i)); }
					ImGui::SameLine();
					if (ImGui::SmallButton("Delete")) { focusableIndexToDelete = static_cast<int>(i); }
					ImGui::PopID();
				}
				if (focusableIndexToDelete != -1) { m_editingRoomDataCopy[U"forcusables"].erase(focusableIndexToDelete); }
				if (ImGui::Button("Add Forcusable...")) { openForcusableEditor(-1); }

				ImGui::Dummy(ImVec2(0.0f, 20.0f));

				// --- Interactable Objects ---
				ImGui::Text("Interactable Objects");
				ImGui::Separator();
				if (not m_editingRoomDataCopy.hasElement(U"interactables")) { m_editingRoomDataCopy[U"interactables"] = Array<JSON>(); }

				int interactableIndexToDelete = -1;
				for (size_t i = 0; i < m_editingRoomDataCopy[U"interactables"].size(); ++i)
				{
					const auto& item = m_editingRoomDataCopy[U"interactables"][i];
					const String name = item[U"name"].getOpt<String>().value_or(U"");
					ImGui::PushID(static_cast<int>(i));
					ImGui::BulletText(name.toUTF8().c_str());
					ImGui::SameLine(ImGui::GetWindowWidth() - 120);
					if (ImGui::SmallButton("Edit")) { openInteractableEditor(static_cast<int>(i)); }
					ImGui::SameLine();
					if (ImGui::SmallButton("Delete")) { interactableIndexToDelete = static_cast<int>(i); }
					ImGui::PopID();
				}
				if (interactableIndexToDelete != -1) { m_editingRoomDataCopy[U"interactables"].erase(interactableIndexToDelete); }
				if (ImGui::Button("Add Interactable...")) { openInteractableEditor(-1); }

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		//==============================================================================
		// 閉じるボタン
		//==============================================================================
		ImGui::Separator();
		if (ImGui::Button("Apply & Save", ImVec2(120, 0)))
		{
			controller.updateRoomData(m_editingRoomName, m_editingRoomDataCopy);
			controller.saveSelectedJson();
			m_showRoomEditor = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			m_showRoomEditor = false;
		}

	}
	ImGui::End();
}


void EditorView::drawMenuBar(EditorController& controller)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Dimension...")) { m_shouldShowNewDimensionPopup = true; }
			if (ImGui::MenuItem("Open Dimension...")) { controller.openDimension(); }
			if (ImGui::MenuItem("Save")) { controller.saveSelectedJson(); }

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
		// 第1階層: Dimension
		const String& dimensionName = model.getDimensionName();
		if (not dimensionName.isEmpty() && ImGui::TreeNode(dimensionName.toUTF8().c_str()))
		{
			const FilePath connectionsPath = model.getCurrentDimensionPath() + U"room_connections.json";
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			if (connectionsPath == controller.getSelectedPath())
			{
				flags |= ImGuiTreeNodeFlags_Selected;
			}
			ImGui::TreeNodeEx("room_connections.json", flags);
			if (ImGui::IsItemClicked())
			{
				controller.setSelectedPath(connectionsPath);
			}

			// 第2階層: Room
			for (const auto& room : model.getRooms())
			{
				if (room.name.isEmpty()) continue;

				ImGuiTreeNodeFlags roomNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
				if (room.objects.isEmpty())
				{
					roomNodeFlags |= ImGuiTreeNodeFlags_Leaf;
				}

				if (ImGui::TreeNodeEx(room.name.toUTF8().c_str(), roomNodeFlags))
				{
					// 第3階層: Object
					for (const auto& object : room.objects)
					{
						if (object.fileName.isEmpty()) continue;

						ImGuiTreeNodeFlags objectNodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

						const FilePath objectPath = model.getCurrentDimensionPath() + U"/" + room.name + U"/" + object.fileName;
						if (objectPath == controller.getSelectedPath())
						{
							objectNodeFlags |= ImGuiTreeNodeFlags_Selected;
						}

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

void EditorView::drawCanvasPanel(EditorController& controller)
{
	ImGui::Begin("Canvas");
	const FilePath& selectedPath = controller.getSelectedPath();

	if (FileSystem::Extension(selectedPath) == U"json")
	{
		const JSON& jsonData = controller.getSelectedJsonData();
		if (not jsonData.isEmpty())
		{
			ImGui::TextUnformatted(jsonData.format(2).toUTF8().c_str());
		}
	}
	else
	{
		ImGui::Text("Visual representation of the room here.");
	}
	ImGui::End();
}

void EditorView::drawInspectorPanel(EditorController& controller)
{
	ImGui::Begin("Inspector");

	const FilePath& selectedPath = controller.getSelectedPath();
	DimensionModel& model = controller.getModel();
	
	if (selectedPath != m_lastSelectedPath)
	{
		m_currentDrawer = InspectorDrawerFactory::Create(FileSystem::BaseName(selectedPath));
		m_lastSelectedPath = selectedPath;
	}

	JSON& jsonData = controller.getSelectedJsonData();

	if (m_currentDrawer && (not jsonData.isEmpty()))
	{
		m_currentDrawer->draw(jsonData, *this, controller, model);

		ImGui::Separator();
		// "hotspots" プロパティを持つスキーマの場合のみボタンを表示
		if (m_currentDrawer && jsonData.hasElement(U"hotspots"))
		{
			if (ImGui::Button("Add Hotspot"))
			{
				m_shouldOpenAddHotspotModal = true;
				m_hotspotDraftState = HotspotDraftState{};
			}
		}

		ImGui::Separator();
		if (ImGui::Button("Save Changes"))
		{
			controller.saveSelectedJson();
		}
	}
	else
	{
		ImGui::Text(selectedPath.isEmpty() ? "No file selected." : "This file type is not yet supported.");
	}
	ImGui::End();
}

void EditorView::drawAddHotspotModal(EditorController& controller)
{
	if (ImGui::BeginPopupModal("Add New Hotspot", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("クリック範囲 (例: A1-B2)");
		ImGui::InputText("##GridPos", &m_hotspotDraftState.gridPosBuffer);

		ImGui::Separator();

		// 作成したカスタムUI描画関数を呼び出す
		ImGui::PushID("RootAction");
		drawCustomActionEditor(m_hotspotDraftState.rootAction);
		ImGui::PopID();

		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			controller.addNewHotspot(m_hotspotDraftState);
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

void EditorView::drawCustomActionEditor(ActionDraft& draft)
{
	// 拡張したACTION_TYPESを使用
	ImGui::Combo("アクションの種類", &draft.typeIndex, ACTION_TYPES, IM_ARRAYSIZE(ACTION_TYPES));

	ImGui::Separator();

	// 選択された種類に応じて、表示するUIを切り替える
	switch (draft.typeIndex)
	{
	case ActionType_ShowText: // テキストを表示
		ImGui::InputText("テキストファイル", &draft.fileBuffer);
		break;

	case ActionType_GiveItem: // アイテムを入手
		ImGui::InputText("入手するアイテムID", &draft.itemBuffer);
		break;

	case ActionType_SetFlag: // フラグを操作
		ImGui::InputText("操作するフラグ名", &draft.flagBuffer);
		ImGui::Checkbox("フラグをONにする", &draft.flagValue);
		break;

	case ActionType_Conditional: // 条件分岐
	{
		ImGui::Text("もし、");
		ImGui::SameLine();
		const char* conditionTypes[] = { "アイテムを持っているなら", "フラグがONなら" };
		ImGui::Combo("##ConditionType", &draft.conditionTypeIndex, conditionTypes, IM_ARRAYSIZE(conditionTypes));

		if (draft.conditionTypeIndex == 0) // アイテム
		{
			ImGui::InputText("アイテムID", &draft.conditionItemBuffer);
		}
		else // フラグ
		{
			ImGui::InputText("フラグ名", &draft.conditionFlagBuffer);
		}

		if (ImGui::TreeNode("成功した時のアクション"))
		{
			if (!draft.successAction) draft.successAction = std::make_unique<ActionDraft>();
			ImGui::PushID("SuccessAction"); // IDを追加
			drawCustomActionEditor(*draft.successAction);
			ImGui::PopID(); // IDを削除
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("失敗した時のアクション"))
		{
			if (!draft.failureAction) draft.failureAction = std::make_unique<ActionDraft>();
			ImGui::PushID("FailureAction"); // IDを追加
			drawCustomActionEditor(*draft.failureAction);
			ImGui::PopID(); // IDを削除
			ImGui::TreePop();
		}
		break;
	}
	case ActionType_Sequence: // 連続実行
	case ActionType_MultiStep: // ステップ実行
	{
		if (draft.typeIndex == ActionType_MultiStep)
		{
			ImGui::InputText("ステップID (重複不可)", &draft.idBuffer);
			ImGui::Separator();
		}

		const char* listLabel = (draft.typeIndex == ActionType_Sequence) ? "実行リスト" : "ステップリスト";
		if (ImGui::TreeNode(listLabel))
		{
			int indexToRemove = -1;
			// unique_ptrのリストをループ
			for (size_t i = 0; i < draft.actionList.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i));
				if (ImGui::Button("X"))
				{
					indexToRemove = static_cast<int>(i);
				}
				ImGui::SameLine();

				// std::to_stringを使用してラベルを生成
				if (ImGui::TreeNode(("Action " + std::to_string(i + 1)).c_str()))
				{
					// unique_ptrの中身を参照で渡す
					drawCustomActionEditor(*draft.actionList[i]);
					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			if (indexToRemove != -1)
			{
				draft.actionList.erase(draft.actionList.begin() + indexToRemove);
			}

			if (ImGui::Button("+ 追加"))
			{
				// 新しいActionDraftをunique_ptrで追加
				draft.actionList.push_back(std::make_unique<ActionDraft>());
			}
			ImGui::TreePop();
		}

		// MultiStepの場合、finalActionの編集UIを表示
		if (draft.typeIndex == ActionType_MultiStep)
		{
			ImGui::Separator();
			if (ImGui::TreeNode("完了後のアクション (任意)"))
			{
				if (!draft.finalAction)
				{
					if (ImGui::Button("アクションを設定"))
					{
						draft.finalAction = std::make_unique<ActionDraft>();
					}
				}

				if (draft.finalAction)
				{
					// 削除ボタンが押されたら即座に削除
					if (ImGui::Button("アクションを削除"))
					{
						draft.finalAction.reset();
					}
					// resetされていなければ編集UIを表示
					else
					{
						ImGui::PushID("FinalAction");
						drawCustomActionEditor(*draft.finalAction);
						ImGui::PopID();
					}
				}
				ImGui::TreePop();
			}
		}
		break;
	}
	case ActionType_ChangeDimension:
	{
		ImGui::InputText("ターゲット次元ID", &draft.targetDimensionBuffer);
		break;
	}
	}
}

void EditorView::buildDraftFromActionJson(ActionDraft& draft, const JSON& json)
{
	if (not json.isObject()) return;

	const String type = json[U"type"].getOpt<String>().value_or(U"");

	if (type == U"ShowText")
	{
		draft.typeIndex = ActionType_ShowText;
		draft.fileBuffer = json[U"file"].getOpt<String>().value_or(U"").toUTF8();
	}
	else if (type == U"GiveItem")
	{
		draft.typeIndex = ActionType_GiveItem;
		draft.itemBuffer = json[U"item"].getOpt<String>().value_or(U"").toUTF8();
	}
	else if (type == U"SetFlag")
	{
		draft.typeIndex = ActionType_SetFlag;
		draft.flagBuffer = json[U"flag"].getOpt<String>().value_or(U"").toUTF8();
		draft.flagValue = json[U"value"].getOpt<bool>().value_or(false);
	}
	else if (type == U"Conditional")
	{
		draft.typeIndex = ActionType_Conditional;
		if (json.hasElement(U"condition"))
		{
			const auto& cond = json[U"condition"];
			if (cond[U"type"].getOpt<String>().value_or(U"") == U"HasItem")
			{
				draft.conditionTypeIndex = 0;
				draft.conditionItemBuffer = cond[U"item"].getOpt<String>().value_or(U"").toUTF8();
			}
			else
			{
				draft.conditionTypeIndex = 1;
				draft.conditionFlagBuffer = cond[U"flag"].getOpt<String>().value_or(U"").toUTF8();
			}
		}

		if (json.hasElement(U"success"))
		{
			draft.successAction = std::make_unique<ActionDraft>();
			buildDraftFromActionJson(*draft.successAction, json[U"success"]);
		}
		if (json.hasElement(U"failure"))
		{
			draft.failureAction = std::make_unique<ActionDraft>();
			buildDraftFromActionJson(*draft.failureAction, json[U"failure"]);
		}
	}
	else if (type == U"Sequence" || type == U"MultiStep")
	{
		draft.typeIndex = (type == U"Sequence") ? ActionType_Sequence : ActionType_MultiStep;
		const String listKey = (type == U"Sequence") ? U"actions" : U"steps";

		if (type == U"MultiStep")
		{
			draft.idBuffer = json[U"id"].getOpt<String>().value_or(U"").toUTF8();
			if (json.hasElement(U"final_action"))
			{
				draft.finalAction = std::make_unique<ActionDraft>();
				buildDraftFromActionJson(*draft.finalAction, json[U"final_action"]);
			}
		}

		if (json.hasElement(listKey))
		{
			for (const auto& action : json[listKey])
			{
				auto subDraft = std::make_unique<ActionDraft>();
				buildDraftFromActionJson(*subDraft, action);
				draft.actionList.push_back(std::move(subDraft));
			}
		}
	}
	else if (type == U"ChangeDimension")
	{
		draft.typeIndex = ActionType_ChangeDimension;
		draft.targetDimensionBuffer = json[U"target"].getOpt<String>().value_or(U"").toUTF8();
	}
}

void EditorView::drawGridSelectorWindow(EditorController& controller)
{
	// 表示フラグがfalseなら、この先の処理は行わない
	if (!m_showGridSelector)
	{
		return;
	}

	// 第2引数に表示フラグの参照を渡すことで、「×」ボタンでウィンドウが閉じられる
	if (ImGui::Begin("Grid Selector", &m_showGridSelector, ImGuiWindowFlags_AlwaysAutoResize))
	{
		const int cols = 16;
		const int rows = 12;
		const float cellSize = 20.0f;
		const ImVec2 buttonSize(cellSize, cellSize);
		const ImVec4 selectColor(0.2f, 0.6f, 1.0f, 0.8f);
		const ImGuiIO& io = ImGui::GetIO();

		// ImGui::GetWindowDrawList()はBegin/Endの内側で呼ぶのが安全
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 p = ImGui::GetCursorScreenPos();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		for (int y = 0; y < rows; ++y)
		{
			for (int x = 0; x < cols; ++x)
			{
				ImGui::PushID(y * cols + x);
				ImGui::InvisibleButton("cell", buttonSize);
				if (ImGui::IsItemClicked())
				{
					if (io.KeyShift)
					{
						if (m_gridDragStartCell.x == -1)
						{
							m_gridDragStartCell = { x, y };
							m_gridSelectionRect.set(x, y, 1, 1);
						}
						else
						{
							Point start = m_gridDragStartCell;
							Point end = { x, y };
							m_gridSelectionRect.set(Min(start.x, end.x), Min(start.y, end.y), Abs(start.x - end.x) + 1, Abs(start.y - end.y) + 1);
							m_gridDragStartCell = { -1, -1 };
						}
					}
					else
					{
						m_gridDragStartCell = { -1, -1 };
						m_gridSelectionRect.set(x, y, 1, 1);
					}
				}
				if (x < cols - 1)
				{
					ImGui::SameLine();
				}
				ImGui::PopID();
			}
		}
		ImGui::PopStyleVar();

		for (int y = 0; y <= rows; ++y) { drawList->AddLine(ImVec2(p.x, p.y + y * cellSize), ImVec2(p.x + cols * cellSize, p.y + y * cellSize), IM_COL32(100, 100, 100, 255)); }
		for (int x = 0; x <= cols; ++x) { drawList->AddLine(ImVec2(p.x + x * cellSize, p.y), ImVec2(p.x + x * cellSize, p.y + rows * cellSize), IM_COL32(100, 100, 100, 255)); }
		if (m_gridSelectionRect.x != -1)
		{
			ImVec2 r_min(p.x + m_gridSelectionRect.x * cellSize, p.y + m_gridSelectionRect.y * cellSize);
			ImVec2 r_max(r_min.x + m_gridSelectionRect.w * cellSize, r_min.y + m_gridSelectionRect.h * cellSize);
			drawList->AddRectFilled(r_min, r_max, ImGui::GetColorU32(selectColor));
		}
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			if (m_gridSelectorTargetBuffer && m_gridSelectionRect.x != -1)
			{
				*m_gridSelectorTargetBuffer = GridRectToString(m_gridSelectionRect).toUTF8();
			}
			m_showGridSelector = false;
		}
		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			m_showGridSelector = false;
		}
	}
	ImGui::End();
}

void EditorView::drawAddTransitionWindow(EditorController& controller)
{
	// 表示フラグがfalseなら何もしない
	if (!m_showAddTransitionWindow)
	{
		return;
	}

	// BeginPopupModal から Begin に変更
	if (ImGui::Begin("Add New Transition", &m_showAddTransitionWindow, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("New Transition");
		ImGui::Separator();
		const char* directions[] = { "Up", "Down", "Left", "Right", "Forward" };
		ImGui::Combo("Direction", &m_newTransitionDirectionIndex, directions, IM_ARRAYSIZE(directions));

		// "Forward"が選択されている時だけ、grid_posの入力欄を表示
		if (strcmp(directions[m_newTransitionDirectionIndex], "Forward") == 0)
		{
			// テキスト入力を読み取り専用にし、「Select...」ボタンを追加
			ImGui::InputText("Grid Position", &m_newTransitionGridPosBuffer, ImGuiInputTextFlags_ReadOnly);
			ImGui::SameLine();
			if (ImGui::Button("Select...##TransitionGrid"))
			{
				m_gridSelectorTargetBuffer = &m_newTransitionGridPosBuffer;
				m_showGridSelector = true;
			}
		}

		ImGui::InputText("To (Room Name)", &m_newTransitionToBuffer);
		const char* types[] = { "Simple", "Conditional" };
		ImGui::Combo("Type", &m_newTransitionTypeIndex, types, IM_ARRAYSIZE(types));
		if (m_newTransitionTypeIndex == 1) // Conditional
		{
			ImGui::InputText("Condition (Flag Name)", &m_newTransitionConditionBuffer);
			const char* scopes[] = { "Dimension", "Global" };
			ImGui::Combo("Scope", &m_newTransitionScopeIndex, scopes, IM_ARRAYSIZE(scopes));
		}
		if (ImGui::Button("OK"))
		{
			const String direction = Unicode::FromUTF8(directions[m_newTransitionDirectionIndex]);
			const String to = Unicode::FromUTF8(m_newTransitionToBuffer);

			if (not to.isEmpty())
			{
				bool isComplex = false;
				JSON transitionObj;
				transitionObj[U"to"] = to;

				// Conditionalかどうか
				if (m_newTransitionTypeIndex == 1) // Conditional
				{
					const String condition = Unicode::FromUTF8(m_newTransitionConditionBuffer);
					if (not condition.isEmpty())
					{
						isComplex = true;
						transitionObj[U"condition"] = condition;
						const char* scopes[] = { "Dimension", "Global" };
						transitionObj[U"scope"] = Unicode::FromUTF8(scopes[m_newTransitionScopeIndex]);
					}
				}

				// Forwardかつgrid_posが指定されているか
				if (direction == U"Forward" && not m_newTransitionGridPosBuffer.empty())
				{
					isComplex = true;
					transitionObj[U"grid_pos"] = Unicode::FromUTF8(m_newTransitionGridPosBuffer);
				}

				if (isComplex)
				{
					// conditionやgrid_posが指定された場合は、オブジェクトとして保存
					m_editingRoomDataCopy[U"transitions"][direction] = transitionObj;
				}
				else
				{
					// それ以外の場合は、単純な文字列として保存
					m_editingRoomDataCopy[U"transitions"][direction] = to;
				}
			}
			m_showAddTransitionWindow = false;
		}
	}
	// Beginに対応するEndを呼ぶ
	ImGui::End();
}

void EditorView::openForcusableEditor(int index)
{
	if (index == -1) {
		m_isEditingForcusable = false;
		m_forcusableDraftState = {};
	}
	else {
		m_isEditingForcusable = true;
		m_editingForcusableIndex = index;
		const auto& item = m_editingRoomDataCopy[U"forcusables"][index];
		m_forcusableDraftState.nameBuffer = item[U"name"].getOpt<String>().value_or(U"").toUTF8();
		m_forcusableDraftState.defaultStateDraft.assetBuffer = item[U"default_state"][U"asset"].getOpt<String>().value_or(U"").toUTF8();
		m_forcusableDraftState.hotspotGridPosBuffer = item[U"hotspot"][U"grid_pos"].getOpt<String>().value_or(U"").toUTF8();

		m_forcusableDraftState.states.clear();
		if (item.hasElement(U"states"))
		{
			for (const auto& stateJson : item[U"states"].arrayView())
			{
				ConditionalStateDraft stateDraft;
				stateDraft.conditionFlagBuffer = stateJson[U"condition_flag"].getOpt<String>().value_or(U"").toUTF8();
				stateDraft.assetBuffer = stateJson[U"asset"].getOpt<String>().value_or(U"").toUTF8();
				m_forcusableDraftState.states.push_back(stateDraft);
			}
		}
	}
	m_showAddForcusableWindow = (index == -1);
	m_showEditForcusableWindow = (index != -1);
}

void EditorView::drawForcusableEditorWindow(EditorController& controller)
{
	if (!m_showAddForcusableWindow && !m_showEditForcusableWindow) return;
	const char* title = m_isEditingForcusable ? "Edit Forcusable" : "Add New Forcusable";
	bool& show_flag = m_isEditingForcusable ? m_showEditForcusableWindow : m_showAddForcusableWindow;
	if (ImGui::Begin(title, &show_flag, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::InputText("Name (Unique ID)", &m_forcusableDraftState.nameBuffer);
		if (ImGui::CollapsingHeader("Default State", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputText("Asset Name", &m_forcusableDraftState.defaultStateDraft.assetBuffer);
		}
		if (ImGui::CollapsingHeader("Hotspot", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputText("Grid Position", &m_forcusableDraftState.hotspotGridPosBuffer, ImGuiInputTextFlags_ReadOnly);
			ImGui::SameLine();
			if (ImGui::Button("Select...")) {
				openGridSelector(m_forcusableDraftState.hotspotGridPosBuffer);
			}
		}

		if (ImGui::CollapsingHeader("Conditional States"))
		{
			int stateToDelete = -1;
			for (size_t i = 0; i < m_forcusableDraftState.states.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i));
				ImGui::Separator();
				ImGui::InputText("Condition Flag", &m_forcusableDraftState.states[i].conditionFlagBuffer);
				ImGui::InputText("Asset Name", &m_forcusableDraftState.states[i].assetBuffer);
				if (ImGui::Button("Delete State"))
				{
					stateToDelete = static_cast<int>(i);
				}
				ImGui::PopID();
			}

			if (stateToDelete != -1)
			{
				m_forcusableDraftState.states.erase(m_forcusableDraftState.states.begin() + stateToDelete);
			}

			ImGui::Separator();
			if (ImGui::Button("Add State"))
			{
				m_forcusableDraftState.states.push_back({});
			}
		}

		ImGui::Separator();
		if (ImGui::Button("OK")) {
			if (m_isEditingForcusable) {
				controller.updateFocusable(m_editingRoomDataCopy, m_editingForcusableIndex, m_forcusableDraftState);
			}
			else {
				controller.addNewFocusable(m_editingRoomDataCopy, m_forcusableDraftState);
			}
			show_flag = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			show_flag = false;
		}
	}
	ImGui::End();
}

void EditorView::drawInteractableEditorWindow(EditorController& controller)
{
	if (!m_showAddInteractableWindow && !m_showEditInteractableWindow) return;
	const char* title = m_isEditingInteractable ? "Edit Interactable" : "Add New Interactable";
	bool& show_flag = m_isEditingInteractable ? m_showEditInteractableWindow : m_showAddInteractableWindow;
	if (ImGui::Begin(title, &show_flag, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::InputText("Name (Unique ID)", &m_interactableDraftState.nameBuffer);
		if (ImGui::CollapsingHeader("Default State", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputText("Asset Name", &m_interactableDraftState.defaultStateDraft.assetBuffer);
			ImGui::InputText("Grid Position", &m_interactableDraftState.defaultStateDraft.gridPosBuffer, ImGuiInputTextFlags_ReadOnly);
			ImGui::SameLine();
			if (ImGui::Button("Select...##Layout")) {
				openGridSelector(m_interactableDraftState.defaultStateDraft.gridPosBuffer);
			}
		}

		if (ImGui::CollapsingHeader("Conditional States"))
		{
			int stateToDelete = -1;
			for (size_t i = 0; i < m_interactableDraftState.states.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i));
				ImGui::Separator();
				ImGui::InputText("Condition Flag", &m_interactableDraftState.states[i].conditionFlagBuffer);
				ImGui::InputText("Asset Name", &m_interactableDraftState.states[i].assetBuffer);
				ImGui::InputText("Grid Position", &m_interactableDraftState.states[i].gridPosBuffer, ImGuiInputTextFlags_ReadOnly);
				ImGui::SameLine();
				if (ImGui::Button("Select...")) {
					openGridSelector(m_interactableDraftState.states[i].gridPosBuffer);
				}
				if (ImGui::Button("Delete State"))
				{
					stateToDelete = static_cast<int>(i);
				}
				ImGui::PopID();
			}

			if (stateToDelete != -1)
			{
				m_interactableDraftState.states.erase(m_interactableDraftState.states.begin() + stateToDelete);
			}

			ImGui::Separator();
			if (ImGui::Button("Add State"))
			{
				m_interactableDraftState.states.push_back({});
			}
		}


		if (ImGui::CollapsingHeader("Hotspot Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputText("Hotspot Grid Position", &m_interactableDraftState.hotspotDraft.gridPosBuffer, ImGuiInputTextFlags_ReadOnly);
			ImGui::SameLine();
			if (ImGui::Button("Select...##Hotspot")) {
				openGridSelector(m_interactableDraftState.hotspotDraft.gridPosBuffer);
			}
			ImGui::Separator();
			ImGui::Text("Action");
			drawCustomActionEditor(m_interactableDraftState.hotspotDraft.rootAction);
		}
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) {
			if (not m_interactableDraftState.nameBuffer.empty()) {
				if (m_isEditingInteractable) {
					controller.updateInteractable(m_editingRoomDataCopy, m_editingInteractableIndex, m_interactableDraftState);
				}
				else {
					controller.addNewInteractable(m_editingRoomDataCopy, m_interactableDraftState);
				}
			}
			show_flag = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			show_flag = false;
		}
	}
	ImGui::End();
}
