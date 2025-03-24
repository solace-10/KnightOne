#pragma once

#include <map>
#include <vector>

#include <core/smart_ptr.hpp>
#include <imgui/imgui.hpp>

#include "encounter_blueprint_types.hpp"
#include "encounter_editor_id_generator.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Encounter);
class StringNode;

DECLARE_SMART_PTR(EncounterEditor);
class EncounterEditor
{
public:
    EncounterEditor();
    ~EncounterEditor();

    void Update();

private:
    void DrawHeader();
    void DrawEncounterList();
    void DrawNodeEditor();
    void DrawNodes();
    void DrawStandardNode(Node* pNode);
    void DrawStringNode(Node* pNode);
    void DrawPins(Node* pNode);
    void DrawPinIcon(const Pin& pin, bool connected);
    void DrawContextMenus();
    ImColor GetIconColor(PinType type) const;
    int GetPinGroupWidth(const std::vector<Pin>& pins) const;
    int GetPinWidth(const Pin& pin) const;
    int GetPinIconSize() const;
    void DrawStringEditor();
    void AddNewEmptyEncounter();
    void LoadEncounter(const std::string& encounterName);
    std::string GetEncounterName(const std::string& path) const;
    void SaveEncounter();
    void RevertEncounter();
    void RepositionNodes();
    void CreateIdGenerator();
    void UpdateEvents();

    ImGuiNodeEditor::EditorContext* m_pContext{nullptr};
    EncounterEditorIdGenerator m_IdGenerator;
    bool m_ShowStringEditor{false};
    bool m_SaveEnqueued{false};
    bool m_LoadEnqueued{false};
    StringNode* m_pSelectedStringNode{nullptr};
    std::string m_NewEncounterName;
    EncounterSharedPtr m_pSelectedEncounter;
    std::map<std::string, EncounterSharedPtr> m_Encounters;
};

inline int EncounterEditor::GetPinIconSize() const
{
    return 24;
}

inline int EncounterEditor::GetPinWidth(const Pin& pin) const
{
    return static_cast<int>(ImGui::GetStyle().ItemSpacing.x) + GetPinIconSize() + ImGui::CalcTextSize(pin.Name.c_str()).x;
}

} // namespace WingsOfSteel::TheBrightestStar