#pragma once

#include "text/RubyText.h"
#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/yaml.h>

// 標準UIとカスタムUIの定義・編集内容を保持し、YAML入出力と表示上書きを管理する。
class UILoadSystem {
public:
    enum class CustomElementType {
        Text,
        Image,
        Panel,
    };

    struct TextureInfo {
        float x = 0.0f;
        float xRatio = 0.0f;
        float y = 0.0f;
        float yRatio = 0.0f;
        float width = 0.0f;
        float widthRatio = 0.0f;
        float height = 0.0f;
        float heightRatio = 0.0f;
        float rotationDegrees = 0.0f;
    };

    struct TextInfo {
        float x = 0.0f;
        float xRatio = 0.0f;
        float y = 0.0f;
        float yRatio = 0.0f;
        float scale = 0.0f;
        float scaleRatio = 0.0f;
        float rubyScaleRatio = 0.48f;
        float rubyGapRatio = 0.0f;
        bool centerBased = false;
        float rotationDegrees = 0.0f;
        std::vector<std::string> texts;
        std::vector<std::vector<RubyTextSegment>> rubySegments;
    };

    struct CustomElement {
        std::string screen = "custom";
        std::string id = "element";
        std::string displayName = "element";
        CustomElementType type = CustomElementType::Text;
        bool visibleByDefault = false;
        bool centerBased = false;
        bool flipVertical = true;
        int zOrder = 0;
        float xRatio = 0.5f;
        float yRatio = 0.28125f;
        float widthRatio = 0.2f;
        float heightRatio = 0.05625f;
        float rotationDegrees = 0.0f;
        float textScaleRatio = 0.0007f;
        std::string text = "New Text";
        std::string texturePath;
        bool usesInputDeviceVariants = false;
        std::string keyboardText;
        std::string gameControllerText;
        std::string keyboardModifierText;
        std::string gameControllerModifierText;
        std::string keyboardTexturePath;
        std::string gameControllerTexturePath;
        bool keyboardFlipVertical = true;
        bool gameControllerFlipVertical = true;
        std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
        bool shadowEnabled = false;
        float shadowOffsetXRatio = 0.0015f;
        float shadowOffsetYRatio = 0.0015f;
        std::array<float, 4> shadowColor = {0.0f, 0.0f, 0.0f, 0.65f};
        bool outlineEnabled = false;
        float outlineWidthRatio = 0.001f;
        std::array<float, 4> outlineColor = {0.0f, 0.0f, 0.0f, 1.0f};
    };

    UILoadSystem();

    void Initialize();

    // 定義への借用ポインターを返す。未登録はnullptr。定義再読込で無効になる。
    const TextureInfo* GetTextureInfo(const std::string& screenName, const std::string& id) const
    {
        const std::string mapId = screenName + "." + id;
        auto infoIterator = mTextureInfo.find(mapId);
        return (infoIterator != mTextureInfo.end()) ? &infoIterator->second : nullptr;
    }

    // 定義への借用ポインターを返す。未登録はnullptr。定義再読込で無効になる。
    const TextInfo* GetTextInfo(const std::string& screenName, const std::string& id) const
    {
        const std::string mapId = screenName + "." + id;
        auto infoIterator = mTextInfo.find(mapId);
        return (infoIterator != mTextInfo.end()) ? &infoIterator->second : nullptr;
    }

    // 保持中のTextInfoからキーを逆引きする。nullptr・該当なしは空文字列。
    std::string FindTextInfoKey(const TextInfo* textInfo) const;

    std::unordered_map<std::string, TextureInfo>& GetEditableTextureInfos() { return mTextureInfo; }
    const std::unordered_map<std::string, TextureInfo>& GetEditableTextureInfos() const { return mTextureInfo; }

    std::unordered_map<std::string, TextInfo>& GetEditableTextInfos() { return mTextInfo; }
    const std::unordered_map<std::string, TextInfo>& GetEditableTextInfos() const { return mTextInfo; }

    // 定義を再読込する。YAML読込・変換の失敗時は以前の定義を復元してfalseを返す。
    bool ReloadUIInfo(const std::string& path = "../assets/data/ui/ui.yaml");
    // 現在の標準UI設定を保存する。解析・書込失敗時はfalseで元ファイルを保持する。
    bool SaveUIInfo(const std::string& path);

    // 既存ファイルの未編集項目を保って、現在の標準UI設定をYAML文字列へ変換する。書込は行わない。
    // 既存定義の読込・解析に失敗した場合はYAML::Exceptionを送出する。
    std::string SerializeUIInfo(const std::string& path) const;

    // 標準・カスタムUIをまとめて保存する。失敗時は置換済みファイルの復旧を試み、理由をoutErrorへ返す。
    // 復旧自体の失敗はoutErrorに復旧用コピーの場所を含めて通知する。
    bool SaveEditorUI(std::string& outError, const std::string& uiPath = "../assets/data/ui/ui.yaml",
                      const std::string& customPath = "../assets/data/ui/custom_ui.yaml") const;

    // テキストとルビを更新する。キー・インデックスが不正ならfalse。ファイルへは保存しない。
    bool UpdateTextInfoContent(const std::string& mapId, std::size_t textIndex, const std::string& text);

    std::vector<CustomElement>& GetCustomElements() { return mCustomElements; }
    const std::vector<CustomElement>& GetCustomElements() const { return mCustomElements; }

    std::string ResolveCustomScreenDisplayName(const std::string& screen) const;
    void SetCustomScreenDisplayName(const std::string& screen, const std::string& displayName);

    // 同一画面内で重複しないIDを付けて追加し、末尾のインデックスを返す。ファイル保存は別途行う。
    std::size_t AddCustomElement(CustomElementType type, const std::string& screen, const std::string& requestedId);

    // 要素を複製して末尾のインデックスを返す。範囲外はnullopt。既存要素への参照は無効になり得る。
    std::optional<std::size_t> DuplicateCustomElement(std::size_t index);

    // 要素とその表示上書きを削除する。範囲外はfalse。後続のインデックスは前へ詰まる。
    bool RemoveCustomElement(std::size_t index);

    // 全定義を読めた場合だけ置き換え、表示上書きを解除する。失敗時はfalseで現在の定義・上書きを維持する。
    // ファイル未作成なら空の定義としてtrueを返す。
    bool LoadCustomUI(const std::string& path = "../assets/data/ui/custom_ui.yaml");
    bool SaveCustomUI(const std::string& path = "../assets/data/ui/custom_ui.yaml") const;

    void SetCustomElementVisible(const std::string& screen, const std::string& id, bool visible);
    void SetCustomScreenVisible(const std::string& screen, bool visible);
    void ClearCustomVisibilityOverrides();

    // 要素ごとの上書き、画面ごとの上書き、visibleByDefaultの順に表示状態を解決する。
    bool IsCustomElementVisible(const CustomElement& element) const;

    static const char* CustomElementTypeToString(CustomElementType type);
    static CustomElementType CustomElementTypeFromString(const std::string& type);

private:
    YAML::Node BuildUIInfo(const std::string& path) const;
    YAML::Node BuildCustomUI() const;
    void LoadUIInfo(const std::string& path);
    void LoadTextureInfo(const std::string& screenName, YAML::Node& node);
    void LoadTextInfo(const std::string& screenName, YAML::Node& node);
    std::string MakeUniqueCustomElementId(const std::string& screen, const std::string& requestedId) const;
    static std::string MakeCustomElementKey(const std::string& screen, const std::string& id);

    std::unordered_map<std::string, TextureInfo> mTextureInfo;
    std::unordered_map<std::string, TextInfo> mTextInfo;
    std::vector<CustomElement> mCustomElements;
    std::unordered_map<std::string, std::string> mCustomScreenDisplayNames;
    std::unordered_map<std::string, bool> mCustomElementVisibilityOverrides;
    std::unordered_map<std::string, bool> mCustomScreenVisibilityOverrides;
};
