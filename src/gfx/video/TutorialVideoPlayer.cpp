#include "gfx/video/TutorialVideoPlayer.h"
#include "gfx/video/TutorialVideoPlayerMacBackend.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propidl.h>
#include <wrl/client.h>
#endif

namespace {
constexpr double MediaFoundationTicksPerSecond = 10000000.0;
constexpr int MaximumDecodedFramesPerUpdate = 12;

std::string FormatPlatformError(const char* operation, long errorCode)
{
    std::ostringstream message;
    message << operation << " failed (0x" << std::hex
            << std::uppercase
            << static_cast<unsigned long>(errorCode) << ")";
    return message.str();
}
} // namespace

class TutorialVideoPlayer::Implementation {
public:
    Implementation();
    ~Implementation();

    bool Play(
        const std::string& playbackKey,
        const std::string& assetRelativePath,
        bool shouldLoop);
    void Stop();
    void Update();

    bool IsPlaying(const std::string& playbackKey) const;
    GLuint GetTextureHandle() const;
    int GetVideoWidth() const;
    int GetVideoHeight() const;
    const std::string& GetLastError() const;

private:
#if defined(_WIN32)
    bool OpenSource(const std::filesystem::path& videoPath);
    bool RefreshOutputFormat();
    bool ReadPendingFrame();
    bool CopyFrameFromTwoDimensionalBuffer(
        IMFMediaBuffer* mediaBuffer);
    bool CopyFrameFromContiguousBuffer(IMFSample* sample);
    bool RestartFromBeginning();
    void UploadPendingFrame();
#endif
    void DeleteTexture();

private:
    std::string mPlaybackKey;
    std::string mAssetRelativePath;
    std::string mLastError;
    GLuint mTextureHandle = 0;
    int mVideoWidth = 0;
    int mVideoHeight = 0;
    bool mShouldLoop = true;
    bool mHasPendingFrame = false;
    bool mReachedEndOfStream = false;
    bool mHasFirstTimestamp = false;
    std::int64_t mFirstTimestamp = 0;
    std::int64_t mPendingTimestamp = 0;
    std::vector<std::uint8_t> mPendingPixels;
    std::chrono::steady_clock::time_point mPlaybackStartTime;

#if defined(_WIN32)
    Microsoft::WRL::ComPtr<IMFSourceReader> mSourceReader;
    bool mShouldUninitializeCom = false;
    bool mMediaFoundationStarted = false;
#elif defined(__APPLE__)
    TutorialVideoPlayerMacBackend* mMacBackend = nullptr;
#endif
};

TutorialVideoPlayer::Implementation::Implementation()
{
#if defined(_WIN32)
    const HRESULT comResult =
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    mShouldUninitializeCom =
        comResult == S_OK || comResult == S_FALSE;
    if (FAILED(comResult) &&
        comResult != RPC_E_CHANGED_MODE) {
        mLastError = FormatPlatformError(
            "CoInitializeEx",
            comResult);
        return;
    }

    const HRESULT startupResult = MFStartup(MF_VERSION);
    if (FAILED(startupResult)) {
        mLastError = FormatPlatformError(
            "MFStartup",
            startupResult);
        return;
    }
    mMediaFoundationStarted = true;
#elif defined(__APPLE__)
    mMacBackend = CreateTutorialVideoPlayerMacBackend();
    if (!mMacBackend) {
        mLastError = "AVFoundation video backend could not be created";
    }
#else
    mLastError =
        "MP4 playback is currently supported only on Windows and macOS";
#endif
}

TutorialVideoPlayer::Implementation::~Implementation()
{
    Stop();
#if defined(_WIN32)
    if (mMediaFoundationStarted) {
        MFShutdown();
    }
    if (mShouldUninitializeCom) {
        CoUninitialize();
    }
#elif defined(__APPLE__)
    DestroyTutorialVideoPlayerMacBackend(mMacBackend);
    mMacBackend = nullptr;
#endif
}

bool TutorialVideoPlayer::Implementation::Play(
    const std::string& playbackKey,
    const std::string& assetRelativePath,
    bool shouldLoop)
{
    if (assetRelativePath.empty()) {
        Stop();
        return false;
    }

    if (IsPlaying(playbackKey) &&
        mAssetRelativePath == assetRelativePath) {
        mShouldLoop = shouldLoop;
#if defined(__APPLE__)
        SetTutorialVideoLoopOnMac(
            mMacBackend,
            shouldLoop);
#endif
        return true;
    }

    Stop();
    mLastError.clear();

#if defined(_WIN32)
    if (!mMediaFoundationStarted) {
        if (mLastError.empty()) {
            mLastError = "Media Foundation is not available";
        }
        return false;
    }

    const std::filesystem::path videoPath =
        (std::filesystem::path("../assets") /
         std::filesystem::path(assetRelativePath))
            .lexically_normal();
    if (!std::filesystem::is_regular_file(videoPath)) {
        mLastError =
            "video file was not found: " + assetRelativePath;
        return false;
    }

    if (!OpenSource(videoPath)) {
        Stop();
        return false;
    }

    mPlaybackKey = playbackKey;
    mAssetRelativePath = assetRelativePath;
    mShouldLoop = shouldLoop;
    mPlaybackStartTime = std::chrono::steady_clock::now();
    return true;
#elif defined(__APPLE__)
    if (!mMacBackend) {
        return false;
    }

    const std::filesystem::path videoPath =
        (std::filesystem::path("../assets") /
         std::filesystem::path(assetRelativePath))
            .lexically_normal();
    if (!std::filesystem::is_regular_file(videoPath)) {
        mLastError =
            "video file was not found: " + assetRelativePath;
        return false;
    }
    const bool started = PlayTutorialVideoOnMac(
        mMacBackend,
        playbackKey,
        videoPath.string(),
        shouldLoop);
    if (started) {
        mPlaybackKey = playbackKey;
        mAssetRelativePath = assetRelativePath;
        mShouldLoop = shouldLoop;
    }
    return started;
#else
    (void)playbackKey;
    (void)shouldLoop;
    return false;
#endif
}

void TutorialVideoPlayer::Implementation::Stop()
{
#if defined(_WIN32)
    mSourceReader.Reset();
    DeleteTexture();
#elif defined(__APPLE__)
    StopTutorialVideoOnMac(mMacBackend);
#else
    DeleteTexture();
#endif
    mPlaybackKey.clear();
    mAssetRelativePath.clear();
    mVideoWidth = 0;
    mVideoHeight = 0;
    mHasPendingFrame = false;
    mReachedEndOfStream = false;
    mHasFirstTimestamp = false;
    mFirstTimestamp = 0;
    mPendingTimestamp = 0;
    mPendingPixels.clear();
}

void TutorialVideoPlayer::Implementation::Update()
{
#if defined(_WIN32)
    if (!mSourceReader || mReachedEndOfStream) {
        return;
    }

    const double elapsedSeconds =
        std::chrono::duration<double>(
            std::chrono::steady_clock::now() -
            mPlaybackStartTime)
            .count();

    for (int decodedFrameCount = 0;
         decodedFrameCount < MaximumDecodedFramesPerUpdate;
         ++decodedFrameCount) {
        if (!mHasPendingFrame && !ReadPendingFrame()) {
            if (!mReachedEndOfStream || !mShouldLoop) {
                return;
            }
            if (!RestartFromBeginning()) {
                return;
            }
            return;
        }

        const double framePresentationSeconds =
            static_cast<double>(
                mPendingTimestamp - mFirstTimestamp) /
            MediaFoundationTicksPerSecond;
        if (framePresentationSeconds > elapsedSeconds) {
            return;
        }

        UploadPendingFrame();
        mHasPendingFrame = false;
        mPendingPixels.clear();
    }
#elif defined(__APPLE__)
    UpdateTutorialVideoOnMac(mMacBackend);
#endif
}

bool TutorialVideoPlayer::Implementation::IsPlaying(
    const std::string& playbackKey) const
{
#if defined(__APPLE__)
    return IsTutorialVideoPlayingOnMac(
        mMacBackend,
        playbackKey);
#else
    return mPlaybackKey == playbackKey &&
           !mAssetRelativePath.empty();
#endif
}

GLuint TutorialVideoPlayer::Implementation::GetTextureHandle() const
{
#if defined(__APPLE__)
    return GetTutorialVideoTextureOnMac(mMacBackend);
#else
    return mTextureHandle;
#endif
}

int TutorialVideoPlayer::Implementation::GetVideoWidth() const
{
#if defined(__APPLE__)
    return GetTutorialVideoWidthOnMac(mMacBackend);
#else
    return mVideoWidth;
#endif
}

int TutorialVideoPlayer::Implementation::GetVideoHeight() const
{
#if defined(__APPLE__)
    return GetTutorialVideoHeightOnMac(mMacBackend);
#else
    return mVideoHeight;
#endif
}

const std::string&
TutorialVideoPlayer::Implementation::GetLastError() const
{
#if defined(__APPLE__)
    if (!mLastError.empty() || !mMacBackend) {
        return mLastError;
    }
    return GetTutorialVideoErrorOnMac(mMacBackend);
#else
    return mLastError;
#endif
}

#if defined(_WIN32)
bool TutorialVideoPlayer::Implementation::OpenSource(
    const std::filesystem::path& videoPath)
{
    Microsoft::WRL::ComPtr<IMFAttributes> sourceReaderAttributes;
    HRESULT result = MFCreateAttributes(
        &sourceReaderAttributes,
        1);
    if (FAILED(result)) {
        mLastError = FormatPlatformError(
            "MFCreateAttributes",
            result);
        return false;
    }

    result = sourceReaderAttributes->SetUINT32(
        MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING,
        TRUE);
    if (FAILED(result)) {
        mLastError = FormatPlatformError(
            "EnableVideoProcessing",
            result);
        return false;
    }

    result = MFCreateSourceReaderFromURL(
        videoPath.wstring().c_str(),
        sourceReaderAttributes.Get(),
        &mSourceReader);
    if (FAILED(result)) {
        mLastError = FormatPlatformError(
            "MFCreateSourceReaderFromURL",
            result);
        return false;
    }

    result = mSourceReader->SetStreamSelection(
        MF_SOURCE_READER_ALL_STREAMS,
        FALSE);
    if (FAILED(result)) {
        mLastError = FormatPlatformError(
            "SetStreamSelection(all)",
            result);
        return false;
    }

    result = mSourceReader->SetStreamSelection(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        TRUE);
    if (FAILED(result)) {
        mLastError = FormatPlatformError(
            "SetStreamSelection(video)",
            result);
        return false;
    }

    Microsoft::WRL::ComPtr<IMFMediaType> outputType;
    result = MFCreateMediaType(&outputType);
    if (FAILED(result)) {
        mLastError = FormatPlatformError(
            "MFCreateMediaType",
            result);
        return false;
    }

    outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    result = mSourceReader->SetCurrentMediaType(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        nullptr,
        outputType.Get());
    if (FAILED(result)) {
        mLastError = FormatPlatformError(
            "SetCurrentMediaType(RGB32)",
            result);
        return false;
    }

    return RefreshOutputFormat();
}

bool TutorialVideoPlayer::Implementation::RefreshOutputFormat()
{
    if (!mSourceReader) {
        return false;
    }

    Microsoft::WRL::ComPtr<IMFMediaType> currentType;
    HRESULT result = mSourceReader->GetCurrentMediaType(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        &currentType);
    if (FAILED(result)) {
        mLastError = FormatPlatformError(
            "GetCurrentMediaType",
            result);
        return false;
    }

    GUID subtype = GUID_NULL;
    result = currentType->GetGUID(
        MF_MT_SUBTYPE,
        &subtype);
    if (FAILED(result) || subtype != MFVideoFormat_RGB32) {
        mLastError =
            "Media Foundation changed to an unsupported pixel format";
        return false;
    }

    UINT32 width = 0;
    UINT32 height = 0;
    result = MFGetAttributeSize(
        currentType.Get(),
        MF_MT_FRAME_SIZE,
        &width,
        &height);
    if (FAILED(result) || width == 0 || height == 0) {
        mLastError = FormatPlatformError(
            "MFGetAttributeSize",
            result);
        return false;
    }

    const int newWidth = static_cast<int>(width);
    const int newHeight = static_cast<int>(height);
    const bool dimensionsChanged =
        mVideoWidth != newWidth || mVideoHeight != newHeight;
    if (dimensionsChanged) {
        DeleteTexture();
        mPendingPixels.clear();
        mHasPendingFrame = false;
    }
    mVideoWidth = newWidth;
    mVideoHeight = newHeight;
    return true;
}

bool TutorialVideoPlayer::Implementation::ReadPendingFrame()
{
    if (!mSourceReader) {
        return false;
    }

    DWORD streamFlags = 0;
    LONGLONG timestamp = 0;
    Microsoft::WRL::ComPtr<IMFSample> sample;
    const HRESULT readResult = mSourceReader->ReadSample(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        nullptr,
        &streamFlags,
        &timestamp,
        &sample);
    if (FAILED(readResult)) {
        mLastError = FormatPlatformError(
            "ReadSample",
            readResult);
        mReachedEndOfStream = true;
        return false;
    }

    if ((streamFlags &
         MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0 &&
        !RefreshOutputFormat()) {
        return false;
    }

    if ((streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
        mReachedEndOfStream = true;
        return false;
    }
    if (!sample) {
        return false;
    }

    Microsoft::WRL::ComPtr<IMFMediaBuffer> mediaBuffer;
    const HRESULT bufferResult =
        sample->GetBufferByIndex(0, &mediaBuffer);
    if (FAILED(bufferResult)) {
        mLastError = FormatPlatformError(
            "GetBufferByIndex",
            bufferResult);
        return false;
    }

    const bool copiedFrame =
        CopyFrameFromTwoDimensionalBuffer(mediaBuffer.Get()) ||
        CopyFrameFromContiguousBuffer(sample.Get());
    if (!copiedFrame) {
        if (mLastError.empty()) {
            mLastError = "decoded video frame could not be copied";
        }
        return false;
    }

    mPendingTimestamp = timestamp;
    if (!mHasFirstTimestamp) {
        mFirstTimestamp = timestamp;
        mHasFirstTimestamp = true;
    }
    mHasPendingFrame = true;
    return true;
}

bool TutorialVideoPlayer::Implementation::
    CopyFrameFromTwoDimensionalBuffer(
        IMFMediaBuffer* mediaBuffer)
{
    if (!mediaBuffer) {
        return false;
    }

    Microsoft::WRL::ComPtr<IMF2DBuffer> twoDimensionalBuffer;
    if (FAILED(mediaBuffer->QueryInterface(
            IID_PPV_ARGS(&twoDimensionalBuffer)))) {
        return false;
    }

    BYTE* firstRow = nullptr;
    LONG sourceStrideBytes = 0;
    const HRESULT lockResult = twoDimensionalBuffer->Lock2D(
        &firstRow,
        &sourceStrideBytes);
    if (FAILED(lockResult) || !firstRow) {
        return false;
    }

    const std::size_t destinationStrideBytes =
        static_cast<std::size_t>(mVideoWidth) * 4U;
    const std::size_t sourceStrideMagnitude =
        static_cast<std::size_t>(
            sourceStrideBytes < 0
                ? -static_cast<std::int64_t>(sourceStrideBytes)
                : sourceStrideBytes);
    const bool hasCompleteRows =
        sourceStrideMagnitude >= destinationStrideBytes;
    if (hasCompleteRows) {
        mPendingPixels.resize(
            destinationStrideBytes *
            static_cast<std::size_t>(mVideoHeight));
        for (int row = 0; row < mVideoHeight; ++row) {
            const BYTE* sourceRow =
                firstRow +
                static_cast<std::ptrdiff_t>(row) *
                    static_cast<std::ptrdiff_t>(sourceStrideBytes);
            std::memcpy(
                mPendingPixels.data() +
                    static_cast<std::size_t>(row) *
                        destinationStrideBytes,
                sourceRow,
                destinationStrideBytes);
        }
    }
    twoDimensionalBuffer->Unlock2D();
    return hasCompleteRows;
}

bool TutorialVideoPlayer::Implementation::
    CopyFrameFromContiguousBuffer(IMFSample* sample)
{
    if (!sample || mVideoHeight <= 0) {
        return false;
    }

    Microsoft::WRL::ComPtr<IMFMediaBuffer> contiguousBuffer;
    const HRESULT bufferResult =
        sample->ConvertToContiguousBuffer(&contiguousBuffer);
    if (FAILED(bufferResult)) {
        mLastError = FormatPlatformError(
            "ConvertToContiguousBuffer",
            bufferResult);
        return false;
    }

    BYTE* sourceBytes = nullptr;
    DWORD maximumLength = 0;
    DWORD currentLength = 0;
    const HRESULT lockResult = contiguousBuffer->Lock(
        &sourceBytes,
        &maximumLength,
        &currentLength);
    if (FAILED(lockResult) || !sourceBytes) {
        mLastError = FormatPlatformError(
            "IMFMediaBuffer::Lock",
            lockResult);
        return false;
    }

    const std::size_t destinationStrideBytes =
        static_cast<std::size_t>(mVideoWidth) * 4U;
    const std::size_t sourceStrideBytes =
        static_cast<std::size_t>(currentLength) /
        static_cast<std::size_t>(mVideoHeight);
    const bool hasCompleteRows =
        sourceStrideBytes >= destinationStrideBytes &&
        static_cast<std::size_t>(currentLength) >=
            sourceStrideBytes *
                static_cast<std::size_t>(mVideoHeight);
    if (hasCompleteRows) {
        mPendingPixels.resize(
            destinationStrideBytes *
            static_cast<std::size_t>(mVideoHeight));
        for (int row = 0; row < mVideoHeight; ++row) {
            std::memcpy(
                mPendingPixels.data() +
                    static_cast<std::size_t>(row) *
                        destinationStrideBytes,
                sourceBytes +
                    static_cast<std::size_t>(row) *
                        sourceStrideBytes,
                destinationStrideBytes);
        }
    }
    contiguousBuffer->Unlock();

    if (!hasCompleteRows) {
        mLastError = "decoded video buffer did not contain complete rows";
    }
    return hasCompleteRows;
}

bool TutorialVideoPlayer::Implementation::RestartFromBeginning()
{
    if (!mSourceReader) {
        return false;
    }

    PROPVARIANT startPosition;
    PropVariantInit(&startPosition);
    startPosition.vt = VT_I8;
    startPosition.hVal.QuadPart = 0;
    const HRESULT seekResult =
        mSourceReader->SetCurrentPosition(
            GUID_NULL,
            startPosition);
    PropVariantClear(&startPosition);
    if (FAILED(seekResult)) {
        mLastError = FormatPlatformError(
            "SetCurrentPosition",
            seekResult);
        return false;
    }

    mSourceReader->Flush(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM);
    mReachedEndOfStream = false;
    mHasPendingFrame = false;
    mHasFirstTimestamp = false;
    mFirstTimestamp = 0;
    mPendingTimestamp = 0;
    mPendingPixels.clear();
    mPlaybackStartTime = std::chrono::steady_clock::now();
    return true;
}

void TutorialVideoPlayer::Implementation::UploadPendingFrame()
{
    if (mPendingPixels.empty() ||
        mVideoWidth <= 0 || mVideoHeight <= 0) {
        return;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (mTextureHandle == 0) {
        glGenTextures(1, &mTextureHandle);
        glBindTexture(GL_TEXTURE_2D, mTextureHandle);
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGB8,
            mVideoWidth,
            mVideoHeight,
            0,
            GL_BGRA,
            GL_UNSIGNED_BYTE,
            mPendingPixels.data());
        return;
    }

    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        mVideoWidth,
        mVideoHeight,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        mPendingPixels.data());
}
#endif

void TutorialVideoPlayer::Implementation::DeleteTexture()
{
    if (mTextureHandle == 0) {
        return;
    }
    glDeleteTextures(1, &mTextureHandle);
    mTextureHandle = 0;
}

TutorialVideoPlayer::TutorialVideoPlayer()
    : mImplementation(std::make_unique<Implementation>())
{
}

TutorialVideoPlayer::~TutorialVideoPlayer() = default;

bool TutorialVideoPlayer::Play(
    const std::string& playbackKey,
    const std::string& assetRelativePath,
    bool shouldLoop)
{
    return mImplementation->Play(
        playbackKey,
        assetRelativePath,
        shouldLoop);
}

void TutorialVideoPlayer::Stop()
{
    mImplementation->Stop();
}

void TutorialVideoPlayer::Update()
{
    mImplementation->Update();
}

bool TutorialVideoPlayer::IsPlaying(
    const std::string& playbackKey) const
{
    return mImplementation->IsPlaying(playbackKey);
}

GLuint TutorialVideoPlayer::GetTextureHandle() const
{
    return mImplementation->GetTextureHandle();
}

int TutorialVideoPlayer::GetVideoWidth() const
{
    return mImplementation->GetVideoWidth();
}

int TutorialVideoPlayer::GetVideoHeight() const
{
    return mImplementation->GetVideoHeight();
}

const std::string& TutorialVideoPlayer::GetLastError() const
{
    return mImplementation->GetLastError();
}
