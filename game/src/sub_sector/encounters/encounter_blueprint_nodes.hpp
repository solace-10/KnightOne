#pragma once

#include "encounter_blueprint_types.hpp"
#include "encounter_editor_id_generator.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class BlueprintNodeFactory
{
public:
    static NodeUniquePtr CreateNode(const std::string& nodeName);
};

class SectorEnteredNode : public Node
{
public:
    SectorEnteredNode();
};

class SectorExitNode : public Node
{
public:
    SectorExitNode();
};

class EncounterStageNode : public Node
{
public:
    EncounterStageNode();
};

class StringNode : public Node
{
public:
    StringNode();
    
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    std::string Value;
};    

} // namespace WingsOfSteel::TheBrightestStar

