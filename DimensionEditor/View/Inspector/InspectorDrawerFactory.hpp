#pragma once
#include "../../SchemaManager.hpp"
#include "SchemaDrivenDrawer.hpp"
#include "GenericDrawer.hpp"
#include "RoomConnectionsDrawer.hpp"

namespace InspectorDrawerFactory
{
	inline std::unique_ptr<IInspectorDrawer> Create(const String& fileName)
	{
		if (fileName == U"room_connections")
		{
			return std::make_unique<RoomConnectionsDrawer>();
		}

		if (auto schema = GetSchema(fileName))
		{
			return std::make_unique<SchemaDrivenDrawer>(schema.value());
		}
		else
		{
			return std::make_unique<GenericDrawer>();
		}
	}
}
