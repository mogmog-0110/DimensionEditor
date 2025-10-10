#pragma once
#include <Siv3D.hpp>

struct Schema;

// プロパティが持つべき情報の定義
struct SchemaProperty
{
	String description;       // プロパティの説明（例：「背景画像」）
	JSONValueType type;       // 期待される型（文字列、数値、配列など）
	bool isRequired;          // このプロパティが必須かどうか

	std::shared_ptr<Schema> childSchema = nullptr;
};

// スキーマ本体（プロパティ名の辞書）
struct Schema : HashTable<String, SchemaProperty>
{
	using HashTable<String, SchemaProperty>::HashTable;
};


inline const auto g_ConditionSchema = std::make_shared<Schema>(Schema{
	{ U"type", { U"Type", JSONValueType::String, true, nullptr } },
	// "HasItem" の場合
	{ U"item", { U"Item ID", JSONValueType::String, false, nullptr } },
	// "IsFlagOn" の場合
	{ U"flag", { U"Flag Name", JSONValueType::String, false, nullptr } },
	{ U"scope", { U"Scope (Global/Dimension)", JSONValueType::String, false, nullptr } },
});

inline std::shared_ptr<Schema> g_ActionSchema = std::make_shared<Schema>();

inline void InitializeSchemas()
{
	*g_ActionSchema = {
		{ U"type", { U"Type", JSONValueType::String, true, nullptr } },
		{ U"file", { U"Text File Path", JSONValueType::String, false, nullptr } },
		{ U"item", { U"Item ID", JSONValueType::String, false, nullptr } },
		{ U"flag", { U"Flag Name", JSONValueType::String, false, nullptr } },
		{ U"value", { U"Set Value", JSONValueType::Bool, false, nullptr } },
		{ U"scope", { U"Scope", JSONValueType::String, false, nullptr } },
		{ U"id", { U"MultiStep ID", JSONValueType::String, false, nullptr } },
		{ U"condition", { U"Condition", JSONValueType::Object, false, g_ConditionSchema } },
		{ U"success", { U"Success Action", JSONValueType::Object, false, g_ActionSchema } },
		{ U"failure", { U"Failure Action", JSONValueType::Object, false, g_ActionSchema } },
		{ U"final_action", { U"Final Action", JSONValueType::Object, false, g_ActionSchema } },
		{ U"actions", { U"Action Sequence", JSONValueType::Array, false, g_ActionSchema } },
		{ U"steps", { U"Multi-Step Actions", JSONValueType::Array, false, g_ActionSchema } },
	};
}

inline void InitializeRecursiveSchemas()
{
	auto& actionSchemaRef = const_cast<Schema&>(*g_ActionSchema);
	actionSchemaRef[U"success"].childSchema = g_ActionSchema;
	actionSchemaRef[U"failure"].childSchema = g_ActionSchema;
	actionSchemaRef[U"actions"].childSchema = g_ActionSchema;
	actionSchemaRef[U"steps"].childSchema = g_ActionSchema;
	actionSchemaRef[U"final_action"].childSchema = g_ActionSchema;
}

// "asset" と "grid_pos" を持つ、オブジェクトの基本的な状態スキーマ
inline const auto g_ObjectStateSchema = std::make_shared<Schema>(Schema{
	{ U"asset", { U"Asset Name", JSONValueType::String, true, nullptr } },
	{ U"grid_pos", { U"Grid Position", JSONValueType::String, false, nullptr } },
});

// "condition_flag" を持つ、条件付きの状態スキーマ
inline const auto g_ConditionalStateSchema = std::make_shared<Schema>(Schema{
	{ U"condition_flag", { U"Condition Flag", JSONValueType::String, true, nullptr } },
	{ U"asset", { U"Asset Name", JSONValueType::String, true, nullptr } },
	{ U"grid_pos", { U"Grid Position", JSONValueType::String, false, nullptr } },
});

inline const auto g_HotspotSchema = std::make_shared<Schema>(Schema{
	{ U"grid_pos", { U"Position (e.g., A1-B2)", JSONValueType::String, true, nullptr } },
	{ U"action", { U"Action", JSONValueType::Object, true, g_ActionSchema } }
});

inline const auto g_MissingPieceSchema = std::make_shared<Schema>(Schema{
		{ U"position", { U"Position [x, y]", JSONValueType::Array, true, nullptr } },
		{ U"item", { U"Required Item ID", JSONValueType::String, true, nullptr } }
	});

inline const auto g_CardCaseAnswerSchema = std::make_shared<Schema>(Schema{
	{ U"code", { U"Code (Array of numbers)", JSONValueType::Array, true, nullptr } },
	{ U"flag", { U"Flag to set on success", JSONValueType::String, true, nullptr } }
});

inline const auto g_LockboxAnswerSchema = std::make_shared<Schema>(Schema{
	{ U"code", { U"Code", JSONValueType::String, true, nullptr } },
	{ U"item", { U"Reward Item", JSONValueType::String, true, nullptr } }
});

inline const auto g_RoomSchema = std::make_shared<Schema>(Schema{
	{ U"background", { U"Background Asset", JSONValueType::String, true, nullptr } },
	{ U"transitions", { U"Room Transitions", JSONValueType::Object, false, nullptr } },
	{ U"interactables", { U"Interactable Objects", JSONValueType::Array, false, nullptr /* g_InteractableSchema */ } }, // 再帰定義を避けるため、実際のchildSchemaはInitialize関数で設定
});

// Interactableオブジェクトのスキーマ
inline const auto g_InteractableSchema = std::make_shared<Schema>(Schema{
	{ U"name", { U"Object Name (Unique)", JSONValueType::String, true, nullptr } },
	{ U"asset", { U"Texture Asset Name", JSONValueType::String, true, nullptr } },
	{ U"hotspot", { U"Hotspot", JSONValueType::Object, true, g_HotspotSchema } },
});

// Forcusableオブジェクトのレイアウトスキーマ
inline const auto g_ForcusableLayoutSchema = std::make_shared<Schema>(Schema{
	// 例: { "Whiteboard": "D3" }
	// キー: Forcusableのファイル名(拡張子なし), 値: グリッド座標
});

// Interactableオブジェクトのレイアウトスキーマ
inline const auto g_InteractableLayoutSchema = std::make_shared<Schema>(Schema{
	// 例: { "Window": "D3-E3" }
	// キー: Interactableのname, 値: グリッド座標
});

// レイアウト全体のスキーマ
inline const auto g_LayoutSchema = std::make_shared<Schema>(Schema{
	{ U"forcusable", { U"Focusable Objects", JSONValueType::Object, true, g_ForcusableLayoutSchema } },
	{ U"interactable", { U"Interactable Objects", JSONValueType::Object, true, g_InteractableLayoutSchema } }
});

// 条件付きTransitionオブジェクトのスキーマ
inline const auto g_ConditionalTransitionSchema = std::make_shared<Schema>(Schema{
	{ U"to", { U"Destination Room", JSONValueType::String, true, nullptr } },
	{ U"condition", { U"Required Flag", JSONValueType::String, true, nullptr } },
	{ U"scope", { U"Flag Scope", JSONValueType::String, true, nullptr } }
});

inline const HashTable<String, Schema> g_Schemas = {
		{
			U"room_connections", *g_RoomSchema
		},
		{
			U"Whiteboard", {
				{ U"name", { U"Object Name", JSONValueType::String, true, nullptr } },
				{ U"hotspot", { U"Hotspot", JSONValueType::Object, true, nullptr /* g_HotspotSchema */ } },
				{ U"default_state", { U"Default State", JSONValueType::Object, true, g_ObjectStateSchema } },
				{ U"states", { U"Conditional States", JSONValueType::Array, false, g_ConditionalStateSchema } },
			}
		},
		{
			U"Lockbox", {
				{ U"name", { U"Object Name", JSONValueType::String, true, nullptr } },
				{ U"hotspot", { U"Hotspot", JSONValueType::Object, true, nullptr /* g_HotspotSchema */ } },
				{ U"default_state", { U"Default State", JSONValueType::Object, true, g_ObjectStateSchema } },
				{ U"states", { U"Conditional States", JSONValueType::Array, false, g_ConditionalStateSchema } },
				{ U"answers", { U"Answer List", JSONValueType::Array, true, g_LockboxAnswerSchema } }
			}
		},
		{
			U"Corpse", {
				{ U"name", { U"Object Name", JSONValueType::String, true, nullptr } },
				{ U"hotspot", { U"Hotspot", JSONValueType::Object, true, nullptr /* g_HotspotSchema */ } },
				{ U"default_state", { U"Default State", JSONValueType::Object, true, g_ObjectStateSchema } },
				{ U"states", { U"Conditional States", JSONValueType::Array, false, g_ConditionalStateSchema } },
			}
		},
		{
			U"Diary", {
				{ U"name", { U"Object Name", JSONValueType::String, true, nullptr } },
				{ U"hotspot", { U"Hotspot", JSONValueType::Object, true, nullptr /* g_HotspotSchema */ } },
				{ U"default_state", { U"Default State", JSONValueType::Object, true, g_ObjectStateSchema } },
				{ U"states", { U"Conditional States", JSONValueType::Array, false, g_ConditionalStateSchema } },
				{ U"pages", { U"Diary Pages (Array of Strings)", JSONValueType::Array, true, nullptr } },
			}
		},
		{
			U"Famicom", {
				{ U"name", { U"Object Name", JSONValueType::String, true, nullptr } },
				{ U"hotspot", { U"Hotspot", JSONValueType::Object, true, nullptr /* g_HotspotSchema */ } },
				{ U"default_state", { U"Default State", JSONValueType::Object, true, g_ObjectStateSchema } },
				{ U"states", { U"Conditional States", JSONValueType::Array, false, g_ConditionalStateSchema } },
				{ U"secret_code", { U"Secret Code", JSONValueType::String, true, nullptr } },
				{ U"success_image", { U"Success Image Path", JSONValueType::String, true, nullptr } },
			}
		},
		{
			U"LightsOutPuzzle", {
				{ U"name", { U"Object Name", JSONValueType::String, true, nullptr } },
				{ U"hotspot", { U"Hotspot", JSONValueType::Object, true, nullptr /* g_HotspotSchema */ } },
				{ U"default_state", { U"Default State", JSONValueType::Object, true, g_ObjectStateSchema } },
				{ U"states", { U"Conditional States", JSONValueType::Array, false, g_ConditionalStateSchema } },
				{ U"initial_grid", { U"Initial Grid (2D Array of 0s/1s)", JSONValueType::Array, true, nullptr } },
			}
		},
		{
			U"Kurotto", {
				{ U"name", { U"Object Name", JSONValueType::String, true, nullptr } },
				{ U"hotspot", { U"Hotspot", JSONValueType::Object, true, nullptr /* g_HotspotSchema */ } },
				{ U"default_state", { U"Default State", JSONValueType::Object, true, g_ObjectStateSchema } },
				{ U"states", { U"Conditional States", JSONValueType::Array, false, g_ConditionalStateSchema } },
				{ U"initial_grid", { U"Initial Grid (2D Array)", JSONValueType::Array, true, nullptr } },
			}
		},
		{
			U"RotatingPuzzle", {
				{ U"name", { U"Object Name", JSONValueType::String, true, nullptr } },
				{ U"hotspot", { U"Hotspot", JSONValueType::Object, true, nullptr /* g_HotspotSchema */ } },
				{ U"default_state", { U"Default State", JSONValueType::Object, true, g_ObjectStateSchema } },
				{ U"states", { U"Conditional States", JSONValueType::Array, false, g_ConditionalStateSchema } },
				{ U"background_texture", { U"Background Texture", JSONValueType::String, false, nullptr } },
				{ U"puzzle_texture", { U"Puzzle Texture ID", JSONValueType::String, true, nullptr } },
				{ U"puzzle_width", { U"Puzzle Width (e.g., 3)", JSONValueType::Number, true, nullptr } },
				{ U"missing_pieces", { U"Missing Pieces Info", JSONValueType::Array, true, g_MissingPieceSchema } }
			}
		},
		{
			U"CardCase", {
				{ U"name", { U"Object Name", JSONValueType::String, true, nullptr } },
				{ U"hotspot", { U"Hotspot", JSONValueType::Object, true, nullptr /* g_HotspotSchema */ } },
				{ U"default_state", { U"Default State", JSONValueType::Object, true, g_ObjectStateSchema } },
				{ U"states", { U"Conditional States", JSONValueType::Array, false, g_ConditionalStateSchema } },
				{ U"texture", { U"Card Textures (Array of strings)", JSONValueType::Array, true, nullptr } },
				{ U"answers", { U"Answer Patterns", JSONValueType::Array, true, g_CardCaseAnswerSchema } }
			}
		},
	{
			U"Drawer", {
				{ U"name", { U"Object Name", JSONValueType::String, true, nullptr } },
				{ U"hotspot", { U"Hotspot", JSONValueType::Object, true, nullptr /* g_HotspotSchema */ } },
				{ U"default_state", { U"Default State", JSONValueType::Object, true, g_ObjectStateSchema } },
				{ U"states", { U"Conditional States", JSONValueType::Array, false, g_ConditionalStateSchema } },
				{ U"bg_open", { U"Background (Open)", JSONValueType::String, false, nullptr } },
				{ U"bg_close", { U"Background (Close)", JSONValueType::String, false, nullptr } },
				{ U"item_open", { U"Item (Open)", JSONValueType::String, false, nullptr } },
				{ U"item_close", { U"Item (Close)", JSONValueType::String, false, nullptr } },
			}
		},
};

inline void InitializeSchemaDependencies()
{
	// g_Schemasは定数なので、初期化時のみの特別な処理としてconst_castを使い、一時的に書き込みを許可します。
	auto& schemasRef = const_cast<HashTable<String, Schema>&>(g_Schemas);

	// g_Schemas内のすべてのスキーマをループ
	for (auto& pair : schemasRef)
	{
		// もしスキーマに "hotspot" というキーがあれば
		if (pair.second.contains(U"hotspot"))
		{
			// その hotspot の childSchema に g_HotspotSchema を設定する
			pair.second.at(U"hotspot").childSchema = g_HotspotSchema;
		}
	}
}


inline Optional<Schema> GetSchema(const String& fileName)
{

	if (g_Schemas.contains(fileName))
	{
		Logger << U" -> Found schema!"; // 見つかった場合
		return g_Schemas.at(fileName);
	}

	Logger << U" -> Schema NOT found."; // 見つからなかった場合
	return none;
}
