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

inline const auto g_HotspotSchema = std::make_shared<Schema>(Schema{
	{ U"grid_pos", { U"Position (e.g., A1-B2)", JSONValueType::String, true, nullptr } },
	{ U"action", { U"Action", JSONValueType::Object, true, g_ActionSchema } }
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

// 部屋全体のスキーマ
inline const auto g_RoomSchema = std::make_shared<Schema>(Schema{
	{ U"background", { U"Background Asset", JSONValueType::String, true, nullptr } },
	{ U"transitions", { U"Room Transitions", JSONValueType::Object, false, nullptr } }, // 値が可変なため特別扱い
	{ U"interactables", { U"Interactable Objects", JSONValueType::Array, false, g_InteractableSchema } },
	{ U"layout", { U"Object Layout", JSONValueType::Object, true, g_LayoutSchema } }
});

// Lockboxの答えスキーマ
inline const auto g_LockboxAnswerSchema = std::make_shared<Schema>(Schema{
	{ U"code", { U"Code", JSONValueType::String, true, nullptr } },
	{ U"item", { U"Reward Item", JSONValueType::String, true, nullptr } }
});

inline const HashTable<String, Schema> g_Schemas = {
	{
		U"room_connections", {
			{ U"rooms", { U"Room Definitions", JSONValueType::Object, true, nullptr }}
		}
	},
	{
		U"Whiteboard", {
			{ U"content_image", { U"Content Texture", JSONValueType::String, true, nullptr } },
		}
	},
	{
		U"Lockbox", {
			{ U"background_texture", { U"Background Texture", JSONValueType::String, false, nullptr } },
			{
				U"answers", { U"Answer List", JSONValueType::Array, true, g_LockboxAnswerSchema }
			}
		}
	},
	{
		U"Corpse", {
			{ U"background_texture", { U"Background Texture", JSONValueType::String, false, nullptr } },
		}
	},
	{
		U"Diary", {
			{ U"background_texture", { U"Background Texture", JSONValueType::String, false, nullptr } },
			{ U"pages", { U"Diary Pages (Array of Strings)", JSONValueType::Array, true, nullptr } },
		}
	},
	{
		U"Famicom", {
			{ U"background_texture", { U"Background Texture", JSONValueType::String, false, nullptr } },
			{ U"secret_code", { U"Secret Code", JSONValueType::String, true, nullptr } },
			{ U"success_image", { U"Success Image Path", JSONValueType::String, true, nullptr } },
		}
	},
	{
		U"LightsOutPuzzle", {
			{ U"background_texture", { U"Background Texture", JSONValueType::String, false, nullptr } },
			{ U"initial_grid", { U"Initial Grid (2D Array of 0s/1s)", JSONValueType::Array, true, nullptr } },
		}
	},
};

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
