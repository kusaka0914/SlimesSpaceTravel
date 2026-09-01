#pragma once

#if defined(__APPLE__)

#include <GL/glew.h>

#include <string>

struct TutorialVideoPlayerMacBackend;

TutorialVideoPlayerMacBackend*
CreateTutorialVideoPlayerMacBackend();
void DestroyTutorialVideoPlayerMacBackend(
    TutorialVideoPlayerMacBackend* backend);

bool PlayTutorialVideoOnMac(
    TutorialVideoPlayerMacBackend* backend,
    const std::string& playbackKey,
    const std::string& videoPath,
    bool shouldLoop);
void StopTutorialVideoOnMac(
    TutorialVideoPlayerMacBackend* backend);
void UpdateTutorialVideoOnMac(
    TutorialVideoPlayerMacBackend* backend);
void SetTutorialVideoLoopOnMac(
    TutorialVideoPlayerMacBackend* backend,
    bool shouldLoop);

bool IsTutorialVideoPlayingOnMac(
    const TutorialVideoPlayerMacBackend* backend,
    const std::string& playbackKey);
GLuint GetTutorialVideoTextureOnMac(
    const TutorialVideoPlayerMacBackend* backend);
int GetTutorialVideoWidthOnMac(
    const TutorialVideoPlayerMacBackend* backend);
int GetTutorialVideoHeightOnMac(
    const TutorialVideoPlayerMacBackend* backend);
const std::string& GetTutorialVideoErrorOnMac(
    const TutorialVideoPlayerMacBackend* backend);

#endif
