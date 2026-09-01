#pragma once

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

template <typename Expected, typename Actual>
void ExpectEqual(
    const Expected& expected,
    const Actual& actual,
    const std::string& expression)
{
    if (expected == actual) {
        return;
    }

    std::ostringstream message;
    message << expression << "\n  expected: " << expected
            << "\n  actual:   " << actual;
    throw TestFailure(message.str());
}

inline void ExpectNear(
    float expected,
    float actual,
    float tolerance,
    const std::string& expression)
{
    if (std::abs(expected - actual) <= tolerance) {
        return;
    }

    std::ostringstream message;
    message << expression << "\n  expected: " << expected
            << " +/- " << tolerance
            << "\n  actual:   " << actual;
    throw TestFailure(message.str());
}

inline void ExpectTrue(bool actual, const std::string& expression)
{
    ExpectEqual(true, actual, expression);
}

inline void ExpectFalse(bool actual, const std::string& expression)
{
    ExpectEqual(false, actual, expression);
}
