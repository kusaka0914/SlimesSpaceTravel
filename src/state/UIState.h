#pragma once

// 操作案内の表示状態と、会話対象・ページ番号を保持する。会話内容の取得や描画は行わない。
class UIState {
public:
    enum class TalkWith { None, Opening, Mother, Doctor, Ending, NPC };

    UIState();

    void IncTalkUIIndex() { ++mTalkUIIndex; }

    // 会話対象を切り替え、ページ番号を先頭の0へ戻す。
    void StartTalkWith(TalkWith talkWith);

    // ページ番号だけを0へ戻す。会話対象と操作案内の表示状態は変更しない。
    void FinishTutorial();

    // ページ番号を0へ戻し、会話対象を None にする。
    void FinishTalkWith();

    // フェードイン開始時にページ番号を-1（未選択）へ戻す。会話対象は変更しない。
    void OnFadeIn();

    void SetIsOperationUIShow(bool isOperationUIShow) { mIsOperationUIShow = isOperationUIShow; }
    void SetTalkUIIndex(int talkUIIndex) { mTalkUIIndex = talkUIIndex; }
    void SetCurrentTalkWith(TalkWith currentTalkWith) { mCurrentTalkWith = currentTalkWith; }

    bool GetIsOperationUIShow() const { return mIsOperationUIShow; }
    int GetTalkUIIndex() const { return mTalkUIIndex; }
    TalkWith GetCurrentTalkWith() const { return mCurrentTalkWith; }

private:
    bool mIsOperationUIShow;
    int mTalkUIIndex;
    TalkWith mCurrentTalkWith;
};
