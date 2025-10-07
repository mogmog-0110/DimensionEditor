#pragma once
#include <Siv3D.hpp>

// ViewとControllerの両方で使われるデータ構造の定義

struct ActionDraft
{
	int typeIndex = 0;
	std::string fileBuffer;
	std::string itemBuffer;
	std::string flagBuffer;
	std::string scopeBuffer = "Dimension";
	bool flagValue = true;
	int conditionTypeIndex = 0;
	std::string conditionItemBuffer;
	std::string conditionFlagBuffer;
	std::unique_ptr<ActionDraft> successAction;
	std::unique_ptr<ActionDraft> failureAction;
	std::unique_ptr<ActionDraft> finalAction;
	std::vector<std::unique_ptr<ActionDraft>> actionList;
	std::string idBuffer;
	std::string targetDimensionBuffer;

	// コピー禁止、ムーブ許可
	ActionDraft() = default;
	~ActionDraft() = default;
	ActionDraft(ActionDraft&&) noexcept = default;
	ActionDraft& operator=(ActionDraft&&) noexcept = default;
	ActionDraft(const ActionDraft&) = delete;
	ActionDraft& operator=(const ActionDraft&) = delete;
};

struct HotspotDraftState
{
	std::string gridPosBuffer = "A1-A1";
	ActionDraft rootAction;
};

struct InteractableDraftState
{
	std::string nameBuffer;
	std::string assetBuffer;
	std::string layoutGridPosBuffer;
	HotspotDraftState hotspotDraft;
};
