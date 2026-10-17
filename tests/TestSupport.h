#pragma once

#include <cmath>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// 検証失敗をテスト実行ループへ伝える。捕捉後も残りのテストは実行を続ける。
class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// 表示名と実行関数を登録順に保持する。
using TestRegistry = std::vector<std::pair<std::string, std::function<void()>>>;

// 不一致時はdescriptionと期待値・実際の値を含むTestFailureを送出する。型には比較とストリーム出力が必要。
template <typename Expected, typename Actual>
void ExpectEqual(const Expected& expected, const Actual& actual, const std::string& description)
{
    if (expected == actual) {
        return;
    }

    std::ostringstream message;
    message << description << "\n  expected: " << expected << "\n  actual:   " << actual;
    throw TestFailure(message.str());
}

// 絶対誤差がtolerance以下なら成功とし、それ以外はTestFailureを送出する。toleranceは非負で指定する。
inline void ExpectNear(float expected, float actual, float tolerance, const std::string& description)
{
    if (std::abs(expected - actual) <= tolerance) {
        return;
    }

    std::ostringstream message;
    message << description << "\n  expected: " << expected << " +/- " << tolerance << "\n  actual:   " << actual;
    throw TestFailure(message.str());
}

inline void ExpectTrue(bool actual, const std::string& description)
{
    ExpectEqual(true, actual, description);
}

inline void ExpectFalse(bool actual, const std::string& description)
{
    ExpectEqual(false, actual, description);
}
