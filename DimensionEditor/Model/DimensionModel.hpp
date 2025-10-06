#pragma once
#include <Siv3D.hpp>

// Forcusableオブジェクトのデータ構造
struct FocusableObjectModel
{
	String fileName; // 例: "Lockbox.json"
};

// ルームのデータ構造
struct RoomModel
{
	String name;
	Array<FocusableObjectModel> objects;
};

class DimensionModel
{
public:
	DimensionModel();

	void CreateNew(const FilePath& baseDir, const String& dimensionName);
	void Load(const FilePath& dimensionPath);

	void CreateNewFocusableFile(const String& roomName, const String& fileName);

	void AddNewRoom(const String& roomName);
	
	const String& getDimensionName() const { return m_dimensionName; }
	const Array<RoomModel>& getRooms() const { return m_rooms; }
	bool isDimensionLoaded() const { return (not m_currentDimensionPath.isEmpty()); }
	void saveJsonForPath(const FilePath& path, const JSON& jsonData);

	const FilePath& getCurrentDimensionPath() const { return m_currentDimensionPath; }

	void addHotspot(const FilePath& targetJsonPath, const JSON& newHotspot);



private:
	FilePath m_currentDimensionPath;
	int m_dimensionId;
	String m_dimensionName;
	Array<RoomModel> m_rooms;
};
