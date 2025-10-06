#pragma once
#include "IInspectorDrawer.hpp"
#include "../../ImGuiHelpers.hpp"

class RoomConnectionsDrawer : public IInspectorDrawer
{
public:
	void draw(JSON& jsonData, EditorView& editorView, EditorController& controller, DimensionModel&) override;

private:
	std::string m_newRoomNameBuffer;
};
