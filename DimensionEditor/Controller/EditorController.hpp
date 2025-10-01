#pragma once

class DimensionModel;

class EditorController
{
public:
	explicit EditorController(DimensionModel& model);
	void update();

	// Viewからの通知を受け取る関数
	void openDimension();
	void createNewDimension(const String& name);
	void saveDimension();
	void setSelectedPath(const FilePath& path);

	const FilePath& getSelectedPath() const { return m_selectedPath; }
private:
	DimensionModel& m_model;
	FilePath m_selectedPath;
};
