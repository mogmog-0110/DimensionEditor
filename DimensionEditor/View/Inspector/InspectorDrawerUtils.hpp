#pragma once
#include <Siv3D.hpp>

struct Schema;

void DrawJsonValueEditor(const String& label, JSON& jsonValue, const std::shared_ptr<Schema>& childSchemaHint);
