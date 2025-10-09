#include "DimensionModel.hpp"
#include "../SchemaManager.hpp"

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

JSON CreateTemplateFromSchema(const Schema& schema)
{
	JSON newJson;
	for (const auto& propPair : schema)
	{
		const auto& key = propPair.first;
		const auto& prop = propPair.second;

		switch (prop.type)
		{
		case JSONValueType::String:
			newJson[key] = U"";
			break;
		case JSONValueType::Number:
			newJson[key] = 0;
			break;
		case JSONValueType::Bool:
			newJson[key] = false;
			break;
		case JSONValueType::Array:
			newJson[key] = Array<JSON>();
			break;
		case JSONValueType::Object:
			// 子スキーマが定義されていれば、再帰的にテンプレートを生成
			if (prop.childSchema)
			{
				newJson[key] = CreateTemplateFromSchema(*prop.childSchema);
			}
			else
			{
				newJson[key] = JSON();
			}
			break;
		default:
			newJson[key] = JSON(); // Null
			break;
		}
	}
	return newJson;
}


JSON GetFocusableTemplate(const String& objectType)
{
	// objectTypeに一致するスキーマを取得
	if (auto schema = GetSchema(objectType))
	{
		// 取得したスキーマからテンプレートを自動生成
		return CreateTemplateFromSchema(schema.value());
	}

	// 不明な種類の場合は空のオブジェクトを返す
	Logger << U"⚠️ Warning: Schema not found for type '{}'. Creating an empty object."_fmt(objectType);
	return JSON();
}


DimensionModel::DimensionModel()
{
}

void DimensionModel::CreateNew(const FilePath& baseDir, const String& dimensionName)
{
	m_currentDimensionPath = FileSystem::PathAppend(baseDir, dimensionName);
	if (FileSystem::Exists(m_currentDimensionPath))
	{
		// 既に存在する場合は何もしない
		Logger << U"Dimension '{}' already exists."_fmt(dimensionName);
		return;
	}

	// デフォルトの部屋フォルダをすべて作成
	const Array<String> defaultRooms = {
		U"North", U"East", U"South", U"West", U"Floor", U"Ceiling", U"Multiverse"
	};

	for (const auto& roomName : defaultRooms)
	{
		FileSystem::CreateDirectories(FileSystem::PathAppend(m_currentDimensionPath, roomName));
	}

	// room_connections.json のテンプレートを生成
	JSON connectionsJson;

	// 各部屋の基本設定
	const JSON emptyLayout = JSON{
		{ U"interactable", JSON() }
	};

	// 主要な4部屋（東西南北）
	connectionsJson[U"rooms"][U"North"] = JSON{
		{ U"background", U"BG_NORTH" },
		{ U"interactables", Array<JSON>() }, 
		{ U"forcusables", Array<JSON>() },
		{ U"layout", emptyLayout }
	};
	connectionsJson[U"rooms"][U"East"] = JSON{
		{ U"background", U"BG_EAST" },
		{ U"interactables", Array<JSON>() },
		{ U"forcusables", Array<JSON>() },
		{ U"layout", emptyLayout }
	};
	connectionsJson[U"rooms"][U"South"] = JSON{
		{ U"background", U"BG_SOUTH" },
		{ U"interactables", Array<JSON>() },
		{ U"forcusables", Array<JSON>() },
		{ U"layout", emptyLayout }
	};
	connectionsJson[U"rooms"][U"West"] = JSON{
		{ U"background", U"BG_WEST" },
		{ U"interactables", Array<JSON>() },
		{ U"forcusables", Array<JSON>() },
		{ U"layout", emptyLayout }
	};

	// その他の部屋
	connectionsJson[U"rooms"][U"Ceiling"] = JSON{
		{ U"background", U"BG_CEILING" },
		{ U"interactables", Array<JSON>() },
		{ U"forcusables", Array<JSON>() },
		{ U"layout", emptyLayout }
	};
	connectionsJson[U"rooms"][U"Floor"] = JSON{
		{ U"background", U"BG_FLOOR" },
		{ U"interactables", Array<JSON>() },
		{ U"forcusables", Array<JSON>() },
		{ U"layout", emptyLayout }
	};
	connectionsJson[U"rooms"][U"Multiverse"] = JSON{
		{ U"background", U"BG_MULTIVERSE" },
		{ U"interactables", Array<JSON>() },
		{ U"forcusables", Array<JSON>() },
		{ U"layout", emptyLayout }
	};

	// ファイルに保存
	const FilePath jsonPath = FileSystem::PathAppend(m_currentDimensionPath, U"room_connections.json");
	connectionsJson.save(jsonPath);

	// 作成したDimensionをエディタに読み込む
	Load(m_currentDimensionPath + U"/");
}
void DimensionModel::Load(const FilePath& dimensionPath)
{
	if (not FileSystem::IsDirectory(dimensionPath)) {
		return;
	}

	m_currentDimensionPath = dimensionPath;
	m_dimensionName = GetFolderNameFromPath(dimensionPath);
	m_rooms.clear();

	// ロード済みの部屋名を記録するセット
	HashSet<String> loadedRoomNames;

	// room_connections.json を優先してロード
	const FilePath connectionsPath = FileSystem::PathAppend(dimensionPath, U"room_connections.json");
	const JSON connections = JSON::Load(connectionsPath);

	// connectionsが存在し、かつ"rooms"要素を持っている場合
	if (connections && connections.hasElement(U"rooms") && connections[U"rooms"].isObject())
	{
		for (const JSONItem& roomPair : connections[U"rooms"])
		{
			if (roomPair.key.isEmpty())
			{
				continue;
			}

			RoomModel currentRoom;
			currentRoom.name = roomPair.key;
			const FilePath roomDirectory = FileSystem::PathAppend(dimensionPath, currentRoom.name);

			// フォルダが存在するか確認
			if (FileSystem::IsDirectory(roomDirectory))
			{
				// 部屋のフォルダ内の.jsonファイル（オブジェクト）を探す
				for (const auto& filePath : FileSystem::DirectoryContents(roomDirectory))
				{
					if (FileSystem::Extension(filePath) == U"json")
					{
						const String fileName = FileSystem::FileName(filePath);

						if (not fileName.isEmpty())
						{
							currentRoom.objects.push_back({ fileName });
						}
						else
						{
							// 例外を投げる代わりにログ出力に変更
							Logger << U"⚠️ Warning: Found a JSON file with an empty name in room directory: " + roomDirectory;
						}
					}
				}
			}
			else
			{
				Logger << U"⚠️ Warning: Room defined in JSON but directory not found: " << currentRoom.name;
			}

			m_rooms.push_back(currentRoom);
			loadedRoomNames.insert(currentRoom.name);
		}
	}

	// ディレクトリをスキャンし、まだロードされていない（JSONに記載がない、またはJSON自体がない）フォルダがあれば追加
	for (const auto& path : FileSystem::DirectoryContents(dimensionPath))
	{
		if (FileSystem::IsDirectory(path))
		{
			const String roomName = FileSystem::BaseName(path);
			// まだロードされていなければ追加
			if (not loadedRoomNames.contains(roomName))
			{
				RoomModel currentRoom;
				currentRoom.name = roomName;

				// フォルダの中の.jsonファイルを探す
				for (const auto& filePath : FileSystem::DirectoryContents(path))
				{
					if (FileSystem::Extension(filePath) == U"json")
					{
						currentRoom.objects.push_back({ FileSystem::FileName(filePath) });
					}
				}
				m_rooms.push_back(currentRoom);
			}
		}
	}
}

void DimensionModel::CreateNewFocusableFile(const String& roomName, const String& fileName)
{
	if (m_currentDimensionPath.isEmpty())
	{
		return;
	}

	const FilePath roomPath = FileSystem::PathAppend(m_currentDimensionPath, roomName);
	const FilePath newFilePath = FileSystem::PathAppend(roomPath, fileName);

	if (FileSystem::Exists(newFilePath))
	{
		Logger << U"File '{}' already exists."_fmt(fileName);
		return;
	}

	const String objectType = FileSystem::BaseName(fileName);
	JSON templateJson = GetFocusableTemplate(objectType);

	if (templateJson.save(newFilePath))
	{
		Logger << U"✅ Created new focusable file: " << newFilePath;
		Load(m_currentDimensionPath);
	}
	else
	{
		Logger << U"🚨 Failed to create file: " << newFilePath;
	}
}

void DimensionModel::AddNewRoom(const String& roomName)
{
	if (m_currentDimensionPath.isEmpty() || roomName.isEmpty())
	{
		return;
	}

	// 既に同じ名前の部屋がないか確認
	for (const auto& room : m_rooms)
	{
		if (room.name == roomName)
		{
			Logger << U"Room '{}' already exists in model."_fmt(roomName);
			return;
		}
	}

	// 1. ディスク上に新しいフォルダを作成
	const FilePath roomPath = FileSystem::PathAppend(m_currentDimensionPath, roomName);
	if (FileSystem::CreateDirectories(roomPath))
	{
		Logger << U"✅ Created new directory: " << roomPath;
	}

	// 2. メモリ上の部屋リストに追加
	m_rooms.push_back({ .name = roomName, .objects = {} });
}

void DimensionModel::saveJsonForPath(const FilePath& path, const JSON& jsonData)
{
	if (path.isEmpty())
	{
		return;
	}

	if (jsonData.save(path))
	{
		Logger << U"✅ Saved: " << path;
	}
	else
	{
		Logger << U"🚨 Failed to save: " << path;
	}
}

void DimensionModel::addHotspot(const FilePath& targetJsonPath, const JSON& newHotspot)
{
	if (targetJsonPath.isEmpty() || not FileSystem::Exists(targetJsonPath))
	{
		return;
	}

	JSON targetJson = JSON::Load(targetJsonPath);
	if (not targetJson)
	{
		return;
	}

	// "hotspots" 配列がなければ作成
	if (not targetJson.hasElement(U"hotspots"))
	{
		targetJson[U"hotspots"] = Array<JSON>();
	}

	// "hotspots" が配列であることを確認して追加
	if (targetJson[U"hotspots"].isArray())
	{
		targetJson[U"hotspots"].push_back(newHotspot);
		// ファイルに保存
		saveJsonForPath(targetJsonPath, targetJson);
	}
}
