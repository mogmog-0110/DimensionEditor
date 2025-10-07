#pragma once
#include "Inspector/InspectorDrawerFactory.hpp"
#include "../ImGuiHelpers.hpp"
#include "../Controller/EditorDrafts.hpp"
#include "../SchemaManager.hpp"

class DimensionModel;
class EditorController;

class EditorView
{
public:
	EditorView();
	void draw(DimensionModel& model, EditorController& controller);

	// 部屋編集モーダルの状態
	JSON m_editingRoomDataCopy;
	String m_editingRoomName;
	bool m_showRoomEditor = false;

private:
	void drawMenuBar(EditorController& controller);

	void drawHierarchyPanel(DimensionModel& model, EditorController& controller);

	void drawCanvasPanel(EditorController& controller);

	void drawInspectorPanel(EditorController& controller);

	void drawAddHotspotModal(EditorController& controller);

	void openInteractableEditor(int index=-1);

	void openFocusableEditor(const String& name, const String& gridPos);

	void openGridSelector(std::string& targetBuffer);

	void drawRoomEditorWindow(EditorController& controller);

	void drawCustomActionEditor(ActionDraft& draft);

	void buildDraftFromActionJson(ActionDraft& draft, const JSON& json);

	void drawGridSelectorWindow(EditorController& controller);

	void drawAddFocusableWindow(EditorController& controller);

	void drawEditFocusableWindow(EditorController& controller);

	void drawAddTransitionWindow(EditorController& controller);

	void drawInteractableEditorWindow(EditorController& controller);

	static int s_selectedActionTypeIndex;
	static int s_selectedConditionTypeIndex;

	// ポップアップ内部で使う状態変数
	int m_newTransitionTypeIndex = 0;
	int m_newTransitionDirectionIndex = 0;
	std::string m_newTransitionToBuffer;
	std::string m_newTransitionConditionBuffer;
	int m_newTransitionScopeIndex = 0;

	std::string m_editingFocusableName;
	std::string m_newFocusableNameBuffer;
	std::string m_newFocusableGridPosBuffer;

	InteractableDraftState m_interactableDraftState;
	bool m_isEditingInteractable = false;
	int m_editingInteractableIndex = -1;

	// グリッドセレクタの状態
	std::string* m_gridSelectorTargetBuffer = nullptr;
	s3d::Point m_gridDragStartCell = { -1, -1 };
	s3d::Rect m_gridSelectionRect = { -1, -1, -1, -1 };

	// その他の状態変数
	bool m_shouldShowNewDimensionPopup = false;
	std::string m_newDimensionPathBuffer;
	std::string m_newDimensionNameBuffer = "dimension";
	bool m_shouldOpenAddHotspotModal = false;
	HotspotDraftState m_hotspotDraftState;
	std::unique_ptr<IInspectorDrawer> m_currentDrawer;
	FilePath m_lastSelectedPath;

	bool m_shouldShowInteractablePopup = false;
	bool m_shouldShowEditFocusablePopup = false;
	bool m_shouldShowAddTransitionPopup = false;
	bool m_shouldShowAddFocusablePopup = false;
	bool m_shouldShowGridSelectorPopup = false;

	
	bool m_showAddTransitionWindow = false;
	bool m_showAddFocusable = false;
	bool m_showGridSelector = false;
	bool m_showAddFocusableWindow = false;
	bool m_showEditFocusableWindow = false;
	bool m_showAddInteractableWindow = false;
	bool m_showEditInteractableWindow = false;

	std::string m_newTransitionGridPosBuffer;

};
