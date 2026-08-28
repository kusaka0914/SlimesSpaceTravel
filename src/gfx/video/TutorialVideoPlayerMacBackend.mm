#include "gfx/video/TutorialVideoPlayerMacBackend.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {
constexpr int MaximumDecodedFramesPerUpdate = 12;

std::string MakeErrorMessage(
    const char* operation,
    NSError* error)
{
    const char* description =
        error && error.localizedDescription
            ? error.localizedDescription.UTF8String
            : "unknown error";
    return std::string(operation) + " failed: " + description;
}
}

struct TutorialVideoPlayerMacBackend {
    __strong AVURLAsset* asset = nil;
    __strong AVAssetReader* reader = nil;
    __strong AVAssetReaderTrackOutput* videoOutput = nil;

    std::string playbackKey;
    std::string videoPath;
    std::string lastError;
    GLuint textureHandle = 0;
    int videoWidth = 0;
    int videoHeight = 0;
    bool shouldLoop = true;
    bool hasPendingFrame = false;
    bool reachedEndOfStream = false;
    bool hasFirstTimestamp = false;
    double firstTimestampSeconds = 0.0;
    double pendingTimestampSeconds = 0.0;
    std::vector<std::uint8_t> pendingPixels;
    std::chrono::steady_clock::time_point playbackStartTime;
};

namespace {
void DeleteTexture(TutorialVideoPlayerMacBackend& backend)
{
    if (backend.textureHandle == 0) {
        return;
    }
    glDeleteTextures(1, &backend.textureHandle);
    backend.textureHandle = 0;
}

bool CreateReader(TutorialVideoPlayerMacBackend& backend)
{
    NSArray<AVAssetTrack*>* videoTracks =
        [backend.asset tracksWithMediaType:AVMediaTypeVideo];
    AVAssetTrack* videoTrack = videoTracks.firstObject;
    if (!videoTrack) {
        backend.lastError = "MP4 does not contain a video track";
        return false;
    }

    NSError* readerError = nil;
    backend.reader =
        [[AVAssetReader alloc]
            initWithAsset:backend.asset
                    error:&readerError];
    if (!backend.reader) {
        backend.lastError = MakeErrorMessage(
            "AVAssetReader",
            readerError);
        return false;
    }

    NSDictionary* outputSettings = @{
        (NSString*)kCVPixelBufferPixelFormatTypeKey:
            @(kCVPixelFormatType_32BGRA)
    };
    backend.videoOutput =
        [[AVAssetReaderTrackOutput alloc]
            initWithTrack:videoTrack
           outputSettings:outputSettings];
    backend.videoOutput.alwaysCopiesSampleData = NO;
    if (![backend.reader canAddOutput:backend.videoOutput]) {
        backend.lastError =
            "AVAssetReader could not add the video output";
        return false;
    }

    [backend.reader addOutput:backend.videoOutput];
    if (![backend.reader startReading]) {
        backend.lastError = MakeErrorMessage(
            "AVAssetReader startReading",
            backend.reader.error);
        return false;
    }

    backend.hasPendingFrame = false;
    backend.reachedEndOfStream = false;
    backend.hasFirstTimestamp = false;
    backend.firstTimestampSeconds = 0.0;
    backend.pendingTimestampSeconds = 0.0;
    backend.pendingPixels.clear();
    backend.playbackStartTime =
        std::chrono::steady_clock::now();
    return true;
}

bool ReadPendingFrame(TutorialVideoPlayerMacBackend& backend)
{
    CMSampleBufferRef sampleBuffer =
        [backend.videoOutput copyNextSampleBuffer];
    if (!sampleBuffer) {
        if (backend.reader.status ==
            AVAssetReaderStatusFailed) {
            backend.lastError = MakeErrorMessage(
                "AVAssetReader",
                backend.reader.error);
        }
        backend.reachedEndOfStream = true;
        return false;
    }

    CVImageBufferRef imageBuffer =
        CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!imageBuffer) {
        CFRelease(sampleBuffer);
        backend.lastError =
            "decoded video sample did not contain an image";
        return false;
    }

    CVPixelBufferLockBaseAddress(
        imageBuffer,
        kCVPixelBufferLock_ReadOnly);
    const std::size_t width =
        CVPixelBufferGetWidth(imageBuffer);
    const std::size_t height =
        CVPixelBufferGetHeight(imageBuffer);
    const std::size_t sourceBytesPerRow =
        CVPixelBufferGetBytesPerRow(imageBuffer);
    const std::uint8_t* sourceBytes =
        static_cast<const std::uint8_t*>(
            CVPixelBufferGetBaseAddress(imageBuffer));

    const std::size_t destinationBytesPerRow = width * 4U;
    backend.pendingPixels.resize(
        destinationBytesPerRow * height);
    for (std::size_t row = 0; row < height; ++row) {
        std::memcpy(
            backend.pendingPixels.data() +
                row * destinationBytesPerRow,
            sourceBytes + row * sourceBytesPerRow,
            destinationBytesPerRow);
    }

    CVPixelBufferUnlockBaseAddress(
        imageBuffer,
        kCVPixelBufferLock_ReadOnly);

    backend.videoWidth = static_cast<int>(width);
    backend.videoHeight = static_cast<int>(height);
    const CMTime timestamp =
        CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    backend.pendingTimestampSeconds =
        CMTimeGetSeconds(timestamp);
    CFRelease(sampleBuffer);

    if (!backend.hasFirstTimestamp) {
        backend.firstTimestampSeconds =
            backend.pendingTimestampSeconds;
        backend.hasFirstTimestamp = true;
    }
    backend.hasPendingFrame = true;
    return true;
}

void UploadPendingFrame(TutorialVideoPlayerMacBackend& backend)
{
    if (backend.pendingPixels.empty() ||
        backend.videoWidth <= 0 || backend.videoHeight <= 0) {
        return;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (backend.textureHandle == 0) {
        glGenTextures(1, &backend.textureHandle);
        glBindTexture(GL_TEXTURE_2D, backend.textureHandle);
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
            backend.videoWidth,
            backend.videoHeight,
            0,
            GL_BGRA,
            GL_UNSIGNED_BYTE,
            backend.pendingPixels.data());
        return;
    }

    glBindTexture(GL_TEXTURE_2D, backend.textureHandle);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        backend.videoWidth,
        backend.videoHeight,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        backend.pendingPixels.data());
}
}

TutorialVideoPlayerMacBackend*
CreateTutorialVideoPlayerMacBackend()
{
    return new TutorialVideoPlayerMacBackend();
}

void DestroyTutorialVideoPlayerMacBackend(
    TutorialVideoPlayerMacBackend* backend)
{
    if (!backend) {
        return;
    }
    StopTutorialVideoOnMac(backend);
    delete backend;
}

bool PlayTutorialVideoOnMac(
    TutorialVideoPlayerMacBackend* backend,
    const std::string& playbackKey,
    const std::string& videoPath,
    bool shouldLoop)
{
    if (!backend) {
        return false;
    }
    if (backend->playbackKey == playbackKey &&
        backend->videoPath == videoPath && backend->reader) {
        backend->shouldLoop = shouldLoop;
        return true;
    }

    StopTutorialVideoOnMac(backend);
    backend->lastError.clear();

    NSString* nativePath =
        [NSString stringWithUTF8String:videoPath.c_str()];
    if (!nativePath) {
        backend->lastError = "video path is not valid UTF-8";
        return false;
    }

    NSURL* videoUrl =
        [NSURL fileURLWithPath:nativePath];
    backend->asset =
        [AVURLAsset URLAssetWithURL:videoUrl options:nil];
    backend->playbackKey = playbackKey;
    backend->videoPath = videoPath;
    backend->shouldLoop = shouldLoop;
    if (!CreateReader(*backend)) {
        StopTutorialVideoOnMac(backend);
        return false;
    }
    return true;
}

void StopTutorialVideoOnMac(
    TutorialVideoPlayerMacBackend* backend)
{
    if (!backend) {
        return;
    }
    if (backend->reader.status == AVAssetReaderStatusReading) {
        [backend->reader cancelReading];
    }
    backend->reader = nil;
    backend->videoOutput = nil;
    backend->asset = nil;
    DeleteTexture(*backend);
    backend->playbackKey.clear();
    backend->videoPath.clear();
    backend->videoWidth = 0;
    backend->videoHeight = 0;
    backend->hasPendingFrame = false;
    backend->reachedEndOfStream = false;
    backend->hasFirstTimestamp = false;
    backend->pendingPixels.clear();
}

void UpdateTutorialVideoOnMac(
    TutorialVideoPlayerMacBackend* backend)
{
    if (!backend || !backend->reader ||
        backend->reachedEndOfStream) {
        return;
    }

    const double elapsedSeconds =
        std::chrono::duration<double>(
            std::chrono::steady_clock::now() -
            backend->playbackStartTime)
            .count();

    for (int decodedFrameCount = 0;
         decodedFrameCount < MaximumDecodedFramesPerUpdate;
         ++decodedFrameCount) {
        if (!backend->hasPendingFrame &&
            !ReadPendingFrame(*backend)) {
            if (!backend->reachedEndOfStream ||
                !backend->shouldLoop) {
                return;
            }
            if (!CreateReader(*backend)) {
                return;
            }
            return;
        }

        const double framePresentationSeconds =
            backend->pendingTimestampSeconds -
            backend->firstTimestampSeconds;
        if (framePresentationSeconds > elapsedSeconds) {
            return;
        }

        UploadPendingFrame(*backend);
        backend->hasPendingFrame = false;
        backend->pendingPixels.clear();
    }
}

void SetTutorialVideoLoopOnMac(
    TutorialVideoPlayerMacBackend* backend,
    bool shouldLoop)
{
    if (backend) {
        backend->shouldLoop = shouldLoop;
    }
}

bool IsTutorialVideoPlayingOnMac(
    const TutorialVideoPlayerMacBackend* backend,
    const std::string& playbackKey)
{
    return backend && backend->reader &&
           backend->playbackKey == playbackKey;
}

GLuint GetTutorialVideoTextureOnMac(
    const TutorialVideoPlayerMacBackend* backend)
{
    return backend ? backend->textureHandle : 0;
}

int GetTutorialVideoWidthOnMac(
    const TutorialVideoPlayerMacBackend* backend)
{
    return backend ? backend->videoWidth : 0;
}

int GetTutorialVideoHeightOnMac(
    const TutorialVideoPlayerMacBackend* backend)
{
    return backend ? backend->videoHeight : 0;
}

const std::string& GetTutorialVideoErrorOnMac(
    const TutorialVideoPlayerMacBackend* backend)
{
    static const std::string noBackendError =
        "AVFoundation video backend is not available";
    return backend ? backend->lastError : noBackendError;
}
