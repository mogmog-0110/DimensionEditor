#include "DimensionModel.hpp"

namespace
{
	String GetFolderNameFromPath(FilePath path)
	{
		if (path.ends_with(U'/') || path.ends_with(U'\\'))
		{
			path.pop_back();
		}

		const size_t lastSlashPos = path.lastIndexOf(U'/');

		if (lastSlashPos != String::npos)
		{
			return path.substr(lastSlashPos + 1);
		}

		return path;
	}
}


DimensionModel::DimensionModel()
{
}

void DimensionModel::CreateNew(const FilePath& baseDir, const String& dimensionName)
{
	m_currentDimensionPath = FileSystem::PathAppend(baseDir, dimensionName);
	if (FileSystem::Exists(m_currentDimensionPath)) { return; }

	const Array<String> defaultRooms = { U"North", U"East", U"South", U"West", U"Floor", U"Ceiling", U"Multiverse"};
	for (const auto& roomName : defaultRooms)
	{
		FileSystem::CreateDirectories(FileSystem::PathAppend(m_currentDimensionPath, roomName));
	}
	JSON json;
	json[U"rooms"] = JSON{};
	for (const auto& roomName : defaultRooms)
	{
		json[U"rooms"][roomName] = JSON{};
		json[U"rooms"][roomName][U"layout"] = JSON{};
	}
	json.save(FileSystem::PathAppend(m_currentDimensionPath, U"room_connections.json"));
	Load(m_currentDimensionPath);
}

void DimensionModel::Load(const FilePath& dimensionPath)
{
	if (not FileSystem::IsDirectory(dimensionPath)) {
		return;
	}

	m_currentDimensionPath = dimensionPath;
	m_dimensionName = GetFolderNameFromPath(dimensionPath);
	m_rooms.clear();

	const FilePath connectionsPath = FileSystem::PathAppend(dimensionPath, U"room_connections.json");
	const JSON connections = JSON::Load(connectionsPath);

	if (not connections) {
		return;
	}

	for (const JSONItem& roomPair : connections[U"rooms"])
	{
		if (roomPair.key.isEmpty())
		{
			continue;
		}

		RoomModel currentRoom;
		currentRoom.name = roomPair.key;
		const FilePath roomDirectory = FileSystem::PathAppend(dimensionPath, currentRoom.name);

		for (const auto& filePath : FileSystem::DirectoryContents(roomDirectory))
		{
			if (FileSystem::Extension(filePath) == U"json")
			{
				const String fileName = FileSystem::BaseName(filePath);

				if (not fileName.isEmpty())
				{
					currentRoom.objects.push_back({ fileName });
				}
				else
				{
					throw Error{ U"Error: Found a JSON file with an empty name in room directory: " + roomDirectory };
				}
			}
		}

		m_rooms.push_back(currentRoom);
	}
}

void DimensionModel::Save()
{
	if (m_currentDimensionPath.isEmpty())
	{
		throw Error{ U"Error: No dimension is currently loaded. Cannot save." };
		return;
	}

	// ここに、現在のm_roomsの内容からJSONを組み立てて、
	// room_connections.jsonや各Focusableのjsonに書き出す処理を実装します。
}
