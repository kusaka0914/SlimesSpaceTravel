#pragma once

#include <GL/glew.h>

#include <memory>
#include <string>

class TutorialVideoPlayer final {
public:
    TutorialVideoPlayer();
    ~TutorialVideoPlayer();

    TutorialVideoPlayer(const TutorialVideoPlayer&) = delete;
    TutorialVideoPlayer& operator=(const TutorialVideoPlayer&) = delete;

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
    class Implementation;
    std::unique_ptr<Implementation> mImplementation;
};
