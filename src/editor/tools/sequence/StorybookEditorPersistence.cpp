#include "editor/tools/sequence/StorybookEditorPersistence.h"

#include "system/UILoadSystem.h"
#include "utils/AtomicFile.h"

#include <array>
#include <exception>
#include <filesystem>

namespace StorybookEditorPersistence {

bool Save(const UILoadSystem& ui, const StorybookConfig& storybook, std::string& outError, const std::string& uiPath,
          const std::string& storybookPath)
{
    outError.clear();
    try {
        // ページ番号で対応する2定義を先に確定し、片側だけ新しい内容へ置き換わるのを防ぐ。
        const std::string uiText = ui.SerializeUIInfo(uiPath);
        const std::string imageText = storybook.Serialize();
        const std::filesystem::path imagePath(storybookPath);
        if (imagePath.has_parent_path()) {
            std::filesystem::create_directories(imagePath.parent_path());
        }
        const std::array files = {AtomicFile::TextWrite{uiPath, uiText}, AtomicFile::TextWrite{imagePath, imageText}};
        return AtomicFile::WriteTextFiles(files, outError) == AtomicFile::WriteResult::Written;
    } catch (const std::exception& error) {
        outError = error.what();
        return false;
    }
}

} // namespace StorybookEditorPersistence
