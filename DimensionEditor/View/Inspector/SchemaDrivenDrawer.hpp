#pragma once
#include "IInspectorDrawer.hpp"
#include "../../SchemaManager.hpp"

class SchemaDrivenDrawer : public IInspectorDrawer
{
public:
	explicit SchemaDrivenDrawer(const Schema& schema);
	void draw(JSON& jsonData, EditorView&, EditorController&, DimensionModel&) override;
private:
	const Schema m_schema;
};
