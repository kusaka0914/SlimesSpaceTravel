#pragma once

class Game;
class UIRenderer;

struct DebugEditorContext {
    Game* game = nullptr;
    UIRenderer* uiRenderer = nullptr;
};