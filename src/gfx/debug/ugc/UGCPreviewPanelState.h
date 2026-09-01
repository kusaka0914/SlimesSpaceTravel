#pragma once

class UGCPreviewPanelState {
public:
    void InitializeWidth(
        float preferredWidth,
        float minimumWidth,
        float maximumWidth);
    void BeginResize();
    void Resize(
        float horizontalMouseDelta,
        float minimumWidth,
        float maximumWidth);
    void SetWidth(
        float width,
        float minimumWidth,
        float maximumWidth);

    float GetWidth() const;
    float GetResizeStartWidth() const;
    bool HasInitializedWidth() const;

private:
    float mWidth = 420.0f;
    float mResizeStartWidth = 420.0f;
    bool mHasInitializedWidth = false;
};
