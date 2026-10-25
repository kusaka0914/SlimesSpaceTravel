#include <filesystem>
#include <fstream>
#include <string>

// 実ゲームを起動せず、Restart helperから受け取った引数と作業ディレクトリーを記録する。
int main(int argc, char* argv[])
{
    for (int index = 1; index + 1 < argc; ++index) {
        if (std::string(argv[index]) != "--restore-editor-session") {
            continue;
        }
        std::ofstream output(std::string(argv[index + 1]) + ".launched");
        output << std::filesystem::current_path().string() << '\n';
        for (int argument = 1; argument < argc; ++argument) {
            output << argv[argument] << '\n';
        }
        output << "complete\n";
        output.close();
        return output ? 0 : 1;
    }
    return 2;
}
