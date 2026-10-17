#pragma once

// プレビューの描画サイズ・視点・編集レイヤーへの追従状態を保持する。
class UGCPreviewState {
public:
    // 要求サイズをピクセル単位で受け取り、幅320〜2048・高さ180〜1152に制限する。
    void SetRequestedRenderSize(int width, int height);
    int GetRequestedRenderWidth() const;
    int GetRequestedRenderHeight() const;

    void AdjustYawRadians(float yawDeltaRadians);
    float GetYawRadians() const;

    void ToggleVerticalView();
    bool IsViewedFromBelow() const;

    void SetEditLayer(int layer);
    int GetEditLayer() const;

    // gridSize * 編集レイヤーの高さへ追従し、ワールドY座標を返す。初回は即時反映し、以降はdeltaSecondsで補間する。
    float UpdateFocusY(float gridSize, float deltaSeconds);
    float GetFocusY() const;

private:
    int mRequestedRenderWidth = 960;
    int mRequestedRenderHeight = 540;
    int mEditLayer = 0;
    float mYawRadians = 0.0f;
    float mFocusY = 0.0f;
    bool mHasFocusY = false;
    bool mIsViewedFromBelow = false;
};
