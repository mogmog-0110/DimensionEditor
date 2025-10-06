#pragma once
#include "EditorDrafts.hpp"


class DimensionModel;
class EditorView;

class EditorController
{
public:
	explicit EditorController(DimensionModel& model);
	void update();

	// Viewからの通知を受け取る関数
	void openDimension();
	void createNewDimension(const String& name, const FilePath& baseDir);

	void setSelectedPath(const FilePath& path);

	const FilePath& getSelectedPath() const { return m_selectedPath; }

	JSON& getSelectedJsonData() { return m_selectedJsonData; }

	void saveSelectedJson();

	void addNewHotspot(const HotspotDraftState& hotspotState);

	void updateRoomData(const String& roomName, const JSON& newRoomData);

	void addNewFocusable(const String& roomName, const String& fileName);

	void addNewInteractable(const String& roomName, const InteractableDraftState& draft);

	void addNewRoom(const String& roomName);

	void updateInteractable(const String& roomName, int interactableIndex, const InteractableDraftState& draft);

	void updateFocusable(const String& roomName, const String& focusableName, const String& newGridPos);

	DimensionModel& getModel() { return m_model; }
	const DimensionModel& getModel() const { return m_model; }

private:
	JSON buildJsonFromState(const HotspotDraftState& state);
	JSON buildActionJson(const ActionDraft& draft);

	DimensionModel& m_model;
	FilePath m_selectedPath;
	JSON m_selectedJsonData;
};
