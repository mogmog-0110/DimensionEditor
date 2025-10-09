#include "EditorController.hpp"
#include "../Model/DimensionModel.hpp"

namespace
{
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

EditorController::EditorController(DimensionModel& model)
	: m_model{ model }
{
}

void EditorController::update()
{
	// 今後、キーボードショートカットなどの処理をここに追加
}

void EditorController::openDimension()
{
	const auto result = Dialog::SelectFolder(U"App/data");
	if (result)
	{
		m_model.Load(result.value());
		m_selectedPath.clear(); // 選択をリセット
	}
}

void EditorController::createNewDimension(const String& name, const FilePath& baseDir)
{
	// Viewから受け取ったパスと名前をModelに渡す
	m_model.CreateNew(baseDir, name);
	m_selectedPath.clear();
}

void EditorController::saveSelectedJson()
{
	m_model.saveJsonForPath(m_selectedPath, m_selectedJsonData);
}

void EditorController::addNewHotspot(const HotspotDraftState& hotspotState)
{
	// UIの状態からJSONデータを組み立てる
	JSON newHotspotJson = buildJsonFromState(hotspotState);

	// Modelにデータの永続化（ファイル保存）を依頼
	m_model.addHotspot(m_selectedPath, newHotspotJson);

	// Controllerが持つ現在のJSONデータも更新し、UIに即時反映させる
	m_selectedJsonData[U"hotspots"].push_back(newHotspotJson);
}

void EditorController::updateRoomData(const String& roomName, const JSON& newRoomData)
{
	if (m_selectedJsonData.hasElement(U"rooms"))
	{
		m_selectedJsonData[U"rooms"][roomName] = newRoomData;
	}
}

void EditorController::setSelectedPath(const FilePath& path)
{
	m_selectedPath = path;
	// もしパスが空でなく、JSONファイルなら、中身を読み込んで保持する
	if ((not m_selectedPath.isEmpty()) && (FileSystem::Extension(m_selectedPath) == U"json"))
	{
		m_selectedJsonData = JSON::Load(m_selectedPath);
	}
	else
	{
		m_selectedJsonData.clear(); // それ以外の場合はクリア
	}
}

JSON EditorController::buildActionJson(const ActionDraft& draft)
{
	JSON actionObj;
	// アクションの種類に応じて分岐
	switch (draft.typeIndex) {
	case ActionType_ShowText: // テキストを表示
		actionObj[U"type"] = U"ShowText";
		actionObj[U"file"] = Unicode::FromUTF8(draft.fileBuffer);
		break;
	case ActionType_GiveItem: // アイテムを入手
		actionObj[U"type"] = U"GiveItem";
		actionObj[U"item"] = Unicode::FromUTF8(draft.itemBuffer);
		break;
	case ActionType_SetFlag: // フラグを操作
		actionObj[U"type"] = U"SetFlag";
		actionObj[U"flag"] = Unicode::FromUTF8(draft.flagBuffer);
		actionObj[U"value"] = draft.flagValue;
		break;
	case ActionType_Conditional: // 条件分岐
	{
		actionObj[U"type"] = U"Conditional";
		JSON conditionObj;
		if (draft.conditionTypeIndex == 0) // HasItem
		{
			conditionObj[U"type"] = U"HasItem";
			conditionObj[U"item"] = Unicode::FromUTF8(draft.conditionItemBuffer);
		}
		else // IsFlagOn
		{
			conditionObj[U"type"] = U"IsFlagOn";
			conditionObj[U"flag"] = Unicode::FromUTF8(draft.conditionFlagBuffer);
		}
		actionObj[U"condition"] = conditionObj;

		if (draft.successAction) {
			actionObj[U"success"] = buildActionJson(*draft.successAction);
		}
		if (draft.failureAction) {
			actionObj[U"failure"] = buildActionJson(*draft.failureAction);
		}
		break;
	}
	case ActionType_Sequence: // 連続実行
	{
		actionObj[U"type"] = U"Sequence";
		Array<JSON> actionsArray;
		// unique_ptrのリストを反復処理し、中身を再帰的に処理
		for (const auto& subDraftPtr : draft.actionList)
		{
			if (subDraftPtr)
			{
				actionsArray.push_back(buildActionJson(*subDraftPtr));
			}
		}
		actionObj[U"actions"] = actionsArray;
		break;
	}
	case ActionType_MultiStep: // ステップ実行
	{
		actionObj[U"type"] = U"MultiStep";
		actionObj[U"id"] = Unicode::FromUTF8(draft.idBuffer);
		Array<JSON> stepsArray;
		for (const auto& subDraftPtr : draft.actionList)
		{
			if (subDraftPtr)
			{
				stepsArray.push_back(buildActionJson(*subDraftPtr));
			}
		}
		actionObj[U"steps"] = stepsArray;

		if (draft.finalAction)
		{
			actionObj[U"final_action"] = buildActionJson(*draft.finalAction);
		}
		break;
	}
	case ActionType_ChangeDimension:
		actionObj[U"type"] = U"ChangeDimension";
		actionObj[U"target"] = Unicode::FromUTF8(draft.targetDimensionBuffer);
		break;
	}
	return actionObj;
}

JSON EditorController::buildJsonFromState(const HotspotDraftState& state)
{
	JSON hotspotObj;
	hotspotObj[U"grid_pos"] = Unicode::FromUTF8(state.gridPosBuffer);
	hotspotObj[U"action"] = buildActionJson(state.rootAction);
	return hotspotObj;
}

void EditorController::addNewInteractable(JSON& roomData, const InteractableDraftState& draft)
{
	JSON newInteractable;
	newInteractable[U"name"] = Unicode::FromUTF8(draft.nameBuffer);
	newInteractable[U"default_state"][U"asset"] = Unicode::FromUTF8(draft.defaultStateDraft.assetBuffer);
	newInteractable[U"default_state"][U"grid_pos"] = Unicode::FromUTF8(draft.defaultStateDraft.gridPosBuffer);

	Array<JSON> statesArray;
	for (const auto& stateDraft : draft.states)
	{
		JSON stateObj;
		stateObj[U"condition_flag"] = Unicode::FromUTF8(stateDraft.conditionFlagBuffer);
		stateObj[U"asset"] = Unicode::FromUTF8(stateDraft.assetBuffer);
		stateObj[U"grid_pos"] = Unicode::FromUTF8(stateDraft.gridPosBuffer);
		statesArray.push_back(stateObj);
	}
	newInteractable[U"states"] = statesArray;
	newInteractable[U"hotspot"] = buildJsonFromState(draft.hotspotDraft);

	if (not roomData.hasElement(U"interactables"))
	{
		roomData[U"interactables"] = Array<JSON>();
	}

	roomData[U"interactables"].push_back(newInteractable);
}

void EditorController::addNewRoom(const String& roomName)
{
	m_model.AddNewRoom(roomName);
}

void EditorController::updateInteractable(JSON& roomData, int interactableIndex, const InteractableDraftState& draft)
{
	auto&& target = roomData[U"interactables"][interactableIndex];
	target[U"name"] = Unicode::FromUTF8(draft.nameBuffer);
	target[U"default_state"][U"asset"] = Unicode::FromUTF8(draft.defaultStateDraft.assetBuffer);
	target[U"default_state"][U"grid_pos"] = Unicode::FromUTF8(draft.defaultStateDraft.gridPosBuffer);

	Array<JSON> statesArray;
	for (const auto& stateDraft : draft.states)
	{
		JSON stateObj;
		stateObj[U"condition_flag"] = Unicode::FromUTF8(stateDraft.conditionFlagBuffer);
		stateObj[U"asset"] = Unicode::FromUTF8(stateDraft.assetBuffer);
		stateObj[U"grid_pos"] = Unicode::FromUTF8(stateDraft.gridPosBuffer);
		statesArray.push_back(stateObj);
	}
	target[U"states"] = statesArray;
	target[U"hotspot"] = buildJsonFromState(draft.hotspotDraft);
}

void EditorController::addNewFocusable(JSON& roomData, const ForcusableDraftState& draft)
{
	JSON newForcusable;
	newForcusable[U"name"] = Unicode::FromUTF8(draft.nameBuffer);
	newForcusable[U"default_state"][U"asset"] = Unicode::FromUTF8(draft.defaultStateDraft.assetBuffer);
	newForcusable[U"hotspot"][U"grid_pos"] = Unicode::FromUTF8(draft.hotspotGridPosBuffer);

	Array<JSON> statesArray;
	for (const auto& stateDraft : draft.states)
	{
		JSON stateObj;
		stateObj[U"condition_flag"] = Unicode::FromUTF8(stateDraft.conditionFlagBuffer);
		stateObj[U"asset"] = Unicode::FromUTF8(stateDraft.assetBuffer);
		statesArray.push_back(stateObj);
	}
	newForcusable[U"states"] = statesArray;

	if (not roomData.hasElement(U"forcusables"))
	{
		roomData[U"forcusables"] = Array<JSON>();
	}

	roomData[U"forcusables"].push_back(newForcusable);
}

void EditorController::updateFocusable(JSON& roomData, int focusableIndex, const ForcusableDraftState& draft)
{
	auto&& target = roomData[U"forcusables"][focusableIndex];
	target[U"name"] = Unicode::FromUTF8(draft.nameBuffer);
	target[U"default_state"][U"asset"] = Unicode::FromUTF8(draft.defaultStateDraft.assetBuffer);
	target[U"hotspot"][U"grid_pos"] = Unicode::FromUTF8(draft.hotspotGridPosBuffer);

	Array<JSON> statesArray;
	for (const auto& stateDraft : draft.states)
	{
		JSON stateObj;
		stateObj[U"condition_flag"] = Unicode::FromUTF8(stateDraft.conditionFlagBuffer);
		stateObj[U"asset"] = Unicode::FromUTF8(stateDraft.assetBuffer);
		statesArray.push_back(stateObj);
	}
	target[U"states"] = statesArray;
}
