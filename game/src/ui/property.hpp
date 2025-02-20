#pragma once

#include <string>
#include <unordered_map>

namespace WingsOfSteel::TheBrightestStar::UI
{

struct BaseProperty
{
    virtual ~BaseProperty() = default;
};

template <typename T>
class Property : public BaseProperty
{
public:
    Property() { value = T{}; }
    Property(T value) : value(value) {}

    operator T&() { return value; }
    operator T() const { return value; }

private:
    T value;
};

class PropertyContainer
{
public:
    PropertyContainer() {}
    ~PropertyContainer() {}

    inline void Add(const std::string& name, BaseProperty* pProperty)
    {
        m_Properties[name] = pProperty;
    }

    template <typename T>
    Property<T>* Get(const std::string& name)
    {
        return nullptr;
    }

private:
    std::unordered_map<std::string, BaseProperty*> m_Properties;
};

} // namespace WingsOfSteel::TheBrightestStar::UI