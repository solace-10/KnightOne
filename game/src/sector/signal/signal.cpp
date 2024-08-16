#include "sector/signal/signal.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Signal::Signal(const glm::vec3& position, const nlohmann::json& signalParameters)
: m_Position(position)
{
    m_Name = signalParameters["name"].get<std::string>();
    m_Difficulty = signalParameters["signal_difficulty"].get<float>();
}

Signal::~Signal()
{

}

const std::string& Signal::GetName() const
{
    return m_Name;
}

const glm::vec3& Signal::GetPosition() const
{
    return m_Position;
}

float Signal::GetSignalDifficulty() const
{
    return m_Difficulty;
}

} // namespace WingsOfSteel::TheBrightestStar