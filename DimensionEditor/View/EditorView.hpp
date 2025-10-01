#pragma once

class DimensionModel;
class EditorController;

class EditorView
{
public:
	EditorView();
	void draw(DimensionModel& model, EditorController& controller);
private:
	void drawMenuBar(EditorController& controller);
	void drawHierarchyPanel(DimensionModel& model, EditorController& controller);
	void drawCanvasPanel();
	void drawInspectorPanel(EditorController& controller);
	void drawAssetsPanel();

	bool m_shouldShowNewDimensionPopup = false;
	char m_newDimensionNameBuffer[128];
};
