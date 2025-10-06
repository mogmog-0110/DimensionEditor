#pragma once
#include <Siv3D.hpp>

class EditorView;
class EditorController;
class DimensionModel;

class IInspectorDrawer
{
public:
	virtual ~IInspectorDrawer() = default;
	virtual void draw(JSON& jsonData, EditorView& editorView, EditorController& controller, DimensionModel& model) = 0;
};
