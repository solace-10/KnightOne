#pragma once

#include <optional>

#include <core/smart_ptr.hpp>

#include "encounter_blueprint_types.hpp"
#include "encounter_editor_id_generator.hpp"

namespace WingsOfSteel::Pandora
{
DECLARE_SMART_PTR(TextEditor);
}

namespace WingsOfSteel::TheBrightestStar
{

class BlueprintNodeFactory
{
public:
    static NodeSharedPtr CreateNode(const std::string& nodeName);
};

DECLARE_SMART_PTR(SectorEnteredNode);
class SectorEnteredNode : public Node
{
public:
    SectorEnteredNode();

    NodeType GetNodeType() const override;
    ExecutionResult Execute(Encounter* pEncounter, float delta) override;
};

DECLARE_SMART_PTR(SectorExitNode);
class SectorExitNode : public Node
{
public:
    SectorExitNode();

    NodeType GetNodeType() const override;
};

DECLARE_SMART_PTR(EncounterStageNode);
class EncounterStageNode : public Node
{
public:
    enum class Option
    {
        Option1,
        Option2,
        Option3
    };

    EncounterStageNode();

    NodeType GetNodeType() const override;

    void OnOptionSelected(Option option);

    void OnExecutionStarted(Encounter* pEncounter) override;
    ExecutionResult Execute(Encounter* pEncounter, float delta) override;

private:
    std::optional<Option> m_SelectedOption;
};

DECLARE_SMART_PTR(EncounterOptionNode);
class EncounterOptionNode : public Node
{
public:
    EncounterOptionNode();

    NodeType GetNodeType() const override;
};

DECLARE_SMART_PTR(StringNode);
class StringNode : public Node
{
public:
    StringNode();

    NodeType GetNodeType() const override;
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    std::string Value;
    Pandora::TextEditorUniquePtr Editor;
};

DECLARE_SMART_PTR(DiceNode);
class DiceNode : public Node
{
public:
    DiceNode();

    NodeType GetNodeType() const override;
};

DECLARE_SMART_PTR(ImageNode);
class ImageNode : public Node
{
public:
    ImageNode();

    NodeType GetNodeType() const override;
};

} // namespace WingsOfSteel::TheBrightestStar

