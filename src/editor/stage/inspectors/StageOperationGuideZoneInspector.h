#pragma once

class OperationGuideZone;

// 画像と説明のセット・表示優先度を編集し、変更時にtrueを返す。保存は呼び出し側が行う。
bool DrawOperationGuideZoneInspector(OperationGuideZone& zone);
