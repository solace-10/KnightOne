#pragma once

#include <cassert>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

using DiceValue = uint32_t;

enum class DiceCategory
{
    Electronics = 0,
    Engineering,
    Navigation,
    Science,
    Warfare
};

enum class DiceModifiers
{
    None = 0
};

DECLARE_SMART_PTR(Dice);
class Dice
{
public:
    Dice(DiceValue value) 
    : m_Value(value)
    {
        assert(value >= 1);
        assert(value <= 6);
    }
    ~Dice() {}

    DiceValue GetValue() const;

private:
    DiceValue m_Value{1};
    DiceModifiers m_Modifiers{DiceModifiers::None};
};

inline DiceValue Dice::GetValue() const
{
    return m_Value;
}

} // namespace WingsOfSteel::TheBrightestStar