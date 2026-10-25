#pragma once

// プレビューパネルの幅とリサイズ開始時の幅を保持する。幅の指定ではminimumWidth <= maximumWidthが必要。
class UGCPreviewPanelState {
public:
    // 初回だけ希望幅を範囲内へ制限して設定する。
    void InitializeWidth(float preferredWidth, float minimumWidth, float maximumWidth);
    void BeginResize();

    // BeginResize時点からの累積マウス移動量で幅を更新する。右方向の移動で幅を縮める。
    void Resize(float horizontalMouseDelta, float minimumWidth, float maximumWidth);
    void SetWidth(float width, float minimumWidth, float maximumWidth);

    float GetWidth() const;
    float GetResizeStartWidth() const;
    bool HasInitializedWidth() const;

private:
    float mWidth = 420.0f;
    float mResizeStartWidth = 420.0f;
    bool mHasInitializedWidth = false;
};
