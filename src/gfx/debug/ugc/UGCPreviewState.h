#pragma once

class UGCPreviewState {
public:
    void SetRequestedRenderSize(int width, int height);
    int GetRequestedRenderWidth() const;
    int GetRequestedRenderHeight() const;

    void AdjustYawRadians(float yawDeltaRadians);
    float GetYawRadians() const;

    void ToggleVerticalView();
    bool IsViewedFromBelow() const;

    void SetEditLayer(int layer);
    int GetEditLayer() const;
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
