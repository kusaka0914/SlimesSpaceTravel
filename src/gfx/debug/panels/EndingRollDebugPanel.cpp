#include "gfx/debug/panels/EndingRollDebugPanel.h"

#include "gfx/UIRenderer.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace {
template <std::size_t BufferSize>
bool DrawStringInput(const char* label, std::string& value, bool multiline = false)
{
    std::array<char, BufferSize> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%s", value.c_str());
    const bool changed = multiline
        ? ImGui::InputTextMultiline(label, buffer.data(), buffer.size(), ImVec2(-1.0f, 120.0f))
        : ImGui::InputText(label, buffer.data(), buffer.size());
    if (changed) {
        value = buffer.data();
    }
    return changed;
}
}

EndingRollDebugPanel::EndingRollDebugPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
    Reload();
}

void EndingRollDebugPanel::Reload()
{
    if (EndingRollConfigIO::Load(mConfig)) {
        mStatus = "ending_roll.yaml を読み込みました";
        mSelectedImageIndex = std::clamp(mSelectedImageIndex, -1, static_cast<int>(mConfig.imageEvents.size()) - 1);
    } else {
        mStatus = "設定ファイルの読み込みに失敗しました";
    }
}

void EndingRollDebugPanel::Draw()
{
    if (ImGui::Button("YAMLへ保存")) {
        mStatus = EndingRollConfigIO::Save(mConfig) ? "保存しました。クリア後のエンドロールにも反映されます" : "保存に失敗しました";
    }
    ImGui::SameLine();
    if (ImGui::Button("再読込")) {
        Reload();
    }
    ImGui::SameLine();
    if (ImGui::Button(mIsPreviewPlaying ? "プレビュー停止" : "プレビュー再生")) {
        mIsPreviewPlaying = !mIsPreviewPlaying;
    }
    ImGui::SameLine();
    if (ImGui::Button("先頭へ")) {
        mPreviewTime = 0.0f;
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(mStatus.c_str());

    if (mIsPreviewPlaying) {
        mPreviewTime += ImGui::GetIO().DeltaTime;
        if (mPreviewTime >= mConfig.totalDuration) {
            mPreviewTime = mConfig.totalDuration;
            mIsPreviewPlaying = false;
        }
    }
    ImGui::SliderFloat("プレビュー時間", &mPreviewTime, 0.0f, std::max(1.0f, mConfig.totalDuration), "%.2f 秒");
    ImGui::Separator();
    ImGui::BeginChild("EndingRollBody", ImVec2(0.0f, 0.0f), false);
    DrawSettings();
    ImGui::SameLine();
    DrawPreview();
    ImGui::EndChild();
}

void EndingRollDebugPanel::DrawSettings()
{
    ImGui::BeginChild("EndingRollSettings", ImVec2(360.0f, 0.0f), true);
    ImGui::TextUnformatted("全体・スタッフロール");
    ImGui::DragFloat("全体の長さ", &mConfig.totalDuration, 0.1f, 1.0f, 600.0f, "%.1f 秒");
    ImGui::DragFloat("テキスト開始", &mConfig.creditsStartTime, 0.1f, 0.0f, 600.0f, "%.1f 秒");
    ImGui::DragFloat("テキスト開始Y", &mConfig.creditsStartYRatio, 0.01f, -2.0f, 2.0f);
    ImGui::DragFloat("流れる速さ", &mConfig.creditsScrollSpeedRatio, 0.001f, 0.001f, 1.0f);
    ImGui::DragFloat(
        "文字サイズ", &mConfig.creditsTextScaleRatio,
        0.000005f, 0.00002f, 0.01f, "%.6f");
    DrawStringInput<8192>("スタッフロール", mConfig.creditsText, true);

    ImGui::Separator();
    ImGui::TextUnformatted("最後の全画面End画像");
    DrawImagePicker("End画像", mConfig.endImagePath);
    ImGui::DragFloat("End表示開始", &mConfig.endImageStartTime, 0.1f, 0.0f, 600.0f, "%.1f 秒");
    ImGui::DragFloat("Endフェードイン", &mConfig.endImageFadeInDuration, 0.01f, 0.0f, 30.0f, "%.2f 秒");
    ImGui::DragFloat("End表示時間", &mConfig.endImageHoldDuration, 0.1f, 0.5f, 600.0f, "%.1f 秒");

    ImGui::Separator();
    ImGui::TextUnformatted("出現画像イベント");
    if (ImGui::Button("画像イベントを追加")) {
        mConfig.imageEvents.emplace_back();
        mSelectedImageIndex = static_cast<int>(mConfig.imageEvents.size()) - 1;
    }
    for (int index = 0; index < static_cast<int>(mConfig.imageEvents.size()); ++index) {
        const EndingRollImageEvent& event = mConfig.imageEvents[index];
        const std::string label = std::to_string(index + 1) + ": " + (event.imagePath.empty() ? "画像未設定" : event.imagePath);
        if (ImGui::Selectable(label.c_str(), index == mSelectedImageIndex)) {
            mSelectedImageIndex = index;
        }
    }
    if (mSelectedImageIndex >= 0 && mSelectedImageIndex < static_cast<int>(mConfig.imageEvents.size())) {
        EndingRollImageEvent& event = mConfig.imageEvents[mSelectedImageIndex];
        ImGui::Separator();
        DrawImagePicker("画像", event.imagePath);
        ImGui::DragFloat("開始", &event.startTime, 0.1f, 0.0f, 600.0f, "%.1f 秒");
        ImGui::DragFloat("表示時間", &event.visibleDuration, 0.05f, 0.01f, 60.0f, "%.2f 秒");
        ImGui::DragFloat("繰り返し間隔", &event.repeatInterval, 0.05f, 0.01f, 600.0f, "%.2f 秒");
        ImGui::DragInt("繰り返し回数", &event.repeatCount, 0.1f, 1, 100);
        ImGui::DragFloat("フェードイン", &event.fadeInDuration, 0.01f, 0.0f, 30.0f, "%.2f 秒");
        ImGui::DragFloat("フェードアウト", &event.fadeOutDuration, 0.01f, 0.0f, 30.0f, "%.2f 秒");
        ImGui::DragFloat("中心X", &event.xRatio, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("中心Y", &event.yRatio, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("幅", &event.widthRatio, 0.01f, 0.01f, 1.0f);
        ImGui::DragFloat("高さ", &event.heightRatio, 0.01f, 0.01f, 1.0f);
        if (ImGui::Button("選択中の画像イベントを複製")) {
            mConfig.imageEvents.insert(
                mConfig.imageEvents.begin() + mSelectedImageIndex + 1,
                event);
            ++mSelectedImageIndex;
        }
        if (ImGui::Button("選択中の画像イベントを削除")) {
            mConfig.imageEvents.erase(mConfig.imageEvents.begin() + mSelectedImageIndex);
            mSelectedImageIndex = std::min(mSelectedImageIndex, static_cast<int>(mConfig.imageEvents.size()) - 1);
        }
    }
    ImGui::EndChild();
}

void EndingRollDebugPanel::DrawImagePicker(const char* label, std::string& imagePath)
{
    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("画像アセットを利用できません");
        return;
    }
    mContext.assetCatalog->EnsureScanned();
    ImGui::PushID(label);
    ImGui::TextWrapped("%s: %s", label, imagePath.empty() ? "未設定" : imagePath.c_str());
    ImGui::Button("画像アセットをここへドロップ", ImVec2(-1.0f, 0.0f));
    std::string droppedPath;
    if (EditorAssetDragDrop::AcceptPath(EditorAssetType::Texture, droppedPath)) {
        imagePath = droppedPath;
    }
    if (ImGui::BeginCombo("画像を選択", imagePath.empty() ? "未設定" : imagePath.c_str())) {
        if (ImGui::Selectable("未設定", imagePath.empty())) {
            imagePath.clear();
        }
        for (const std::string& asset :
             mContext.assetCatalog->GetPaths(EditorAssetType::Texture)) {
            if (ImGui::Selectable(asset.c_str(), imagePath == asset)) {
                imagePath = asset;
            }
        }
        ImGui::EndCombo();
    }
    if (!imagePath.empty() && mContext.uiRenderer &&
        mContext.uiRenderer->RegisterCustomUITexture(imagePath)) {
        ImGui::Image(
            static_cast<ImTextureID>(
                mContext.uiRenderer->GetCustomUITextureHandle(imagePath)),
            ImVec2(180.0f, 101.0f), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    }
    ImGui::PopID();
}

void EndingRollDebugPanel::DrawPreview()
{
    ImGui::BeginChild("EndingRollPreview", ImVec2(0.0f, 0.0f), true);
    ImGui::TextUnformatted("実行時プレビュー（画像パスは assets からの相対パス。例: textures/end.png）");
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float previewWidth = std::max(240.0f, available.x);
    const float previewHeight = std::max(
        180.0f,
        std::min(std::max(180.0f, available.y - 28.0f), previewWidth * 9.0f / 16.0f));
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(origin, ImVec2(origin.x + previewWidth, origin.y + previewHeight), IM_COL32(0, 0, 0, 255));
    drawList->PushClipRect(origin, ImVec2(origin.x + previewWidth, origin.y + previewHeight), true);
    const bool showEnd = mPreviewTime >= mConfig.endImageStartTime && !mConfig.endImagePath.empty();
    if (showEnd && mContext.uiRenderer && mContext.uiRenderer->RegisterCustomUITexture(mConfig.endImagePath)) {
        const GLuint texture = mContext.uiRenderer->GetCustomUITextureHandle(mConfig.endImagePath);
        const float elapsed = mPreviewTime - mConfig.endImageStartTime;
        const float opacity = mConfig.endImageFadeInDuration > 0.0f
            ? std::clamp(elapsed / mConfig.endImageFadeInDuration, 0.0f, 1.0f) : 1.0f;
        drawList->AddImage(static_cast<ImTextureID>(texture), origin,
                           ImVec2(origin.x + previewWidth, origin.y + previewHeight),
                           ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f),
                           IM_COL32(255, 255, 255, static_cast<int>(opacity * 255.0f)));
    } else {
        for (const EndingRollImageEvent& event : mConfig.imageEvents) {
            if (!IsEndingRollImageVisible(event, mPreviewTime) || !mContext.uiRenderer ||
                !mContext.uiRenderer->RegisterCustomUITexture(event.imagePath)) {
                continue;
            }
            const float width = previewWidth * event.widthRatio;
            const float height = previewHeight * event.heightRatio;
            const ImVec2 min(origin.x + previewWidth * event.xRatio - width * 0.5f,
                             origin.y + previewHeight * event.yRatio - height * 0.5f);
            const GLuint texture = mContext.uiRenderer->GetCustomUITextureHandle(event.imagePath);
            const int alpha = static_cast<int>(CalculateEndingRollImageOpacity(event, mPreviewTime) * 255.0f);
            drawList->AddImage(static_cast<ImTextureID>(texture), min,
                               ImVec2(min.x + width, min.y + height),
                               ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f),
                               IM_COL32(255, 255, 255, alpha));
        }
        const float y = origin.y + previewHeight *
            (mConfig.creditsStartYRatio - std::max(0.0f, mPreviewTime - mConfig.creditsStartTime) * mConfig.creditsScrollSpeedRatio);
        drawList->AddText(ImGui::GetFont(), std::max(12.0f, previewWidth * mConfig.creditsTextScaleRatio),
                          ImVec2(origin.x + previewWidth * 0.5f - 120.0f, y), IM_COL32_WHITE,
                          mConfig.creditsText.c_str());
    }
    drawList->PopClipRect();
    drawList->AddRect(origin, ImVec2(origin.x + previewWidth, origin.y + previewHeight), IM_COL32(90, 150, 220, 255));
    ImGui::Dummy(ImVec2(previewWidth, previewHeight));
    ImGui::EndChild();
}
