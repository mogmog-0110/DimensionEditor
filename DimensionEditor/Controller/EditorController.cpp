#include "EditorController.hpp"
#include "../Model/DimensionModel.hpp"

EditorController::EditorController(DimensionModel& model) : m_model{ model } {}

void EditorController::update()
{
	// 今後、キーボードショートカットなどの処理をここに追加
}

void EditorController::openDimension()
{
	const auto result = Dialog::SelectFolder(U"App/data");
	if (result)
	{
		m_model.Load(result.value());
		m_selectedPath.clear(); // 選択をリセット
	}
}

void EditorController::createNewDimension(const String& name)
{
	m_model.CreateNew(U"App/data", name);
	m_selectedPath.clear();
}

void EditorController::saveDimension()
{
	m_model.Save();
}

void EditorController::setSelectedPath(const FilePath& path)
{
	m_selectedPath = path;
}
