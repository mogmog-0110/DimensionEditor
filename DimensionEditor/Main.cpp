#include <Siv3D.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui-s3d-wrapper/imgui/DearImGuiAddon.hpp"

#include "Model/DimensionModel.hpp"
#include "View/EditorView.hpp"
#include "Controller/EditorController.hpp"

void Main()
{
	Window::SetTitle(U"Dimension Editor");
	Window::Resize(1280, 720);
	Scene::SetBackground(Palette::White);

	// Siv3DのアドオンとしてImGuiを登録
	Addon::Register<DearImGuiAddon>(U"ImGui");

	// ImGuiのコンテキストを取得して、ドッキングを有効化
	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	InitializeSchemas();
	InitializeRecursiveSchemas();
	InitializeSchemaDependencies();

	DimensionModel model;
	EditorController controller{ model };
	EditorView view;

	while (System::Update())
	{
		// Controllerで入力を処理し、Modelを更新
		controller.update();

		// ViewがModelの状態を描画
		view.draw(model, controller);
	}
}
