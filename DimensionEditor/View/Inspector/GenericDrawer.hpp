#pragma once
#include "IInspectorDrawer.hpp"

class GenericDrawer : public IInspectorDrawer
{
public:
	void draw(JSON& jsonData, EditorView&, EditorController&, DimensionModel&) override;
};
