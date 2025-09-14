#include <glm/gtc/matrix_transform.hpp>

#include "components/faction_component.hpp"
#include "components/wing_component.hpp"
#include "entity_builder/entity_builder.hpp"
#include "sector/encounter.hpp"
#include "sector/sector.hpp"
#include "sector/wing.hpp"

namespace WingsOfSteel
{

Encounter::Encounter()
{

}

Encounter::~Encounter()
{

}

void Encounter::Initialize(SectorSharedPtr pSector)
{
    m_pSector = pSector;
    SpawnCarrier();
}

void Encounter::SpawnCarrier()
{
    SceneWeakPtr pWeakScene = m_pSector;
    SectorWeakPtr pWeakSector = m_pSector;
    EntityBuilder::Build(pWeakScene, "/entity_prefabs/raiders/carrier.json", glm::translate(glm::mat4(1.0f), glm::vec3(250.0f, 0.0f, 0.0f)), [pWeakSector](EntitySharedPtr pEntity){
        pEntity->AddComponent<FactionComponent>().Value = Faction::Hostile;

        SectorSharedPtr pSector = pWeakSector.lock();
        if (pSector)
        {
            Encounter* pEncounter = pSector->GetEncounter();
            pEncounter->m_pCarrier = pEntity;
            pEncounter->RebuildTier(0);
        }
    });
}

void Encounter::RebuildTier(int tier)
{
    m_EncounterTiers[tier].actions.clear();
 
    EncounterWingDescription wingDescription;
    for (int i = 0; i < 5; i++)
    {
        wingDescription.entities.push_back("/entity_prefabs/raiders/interceptor.json");
    }

    EncounterAction action;
    action.wings.push_back(wingDescription);

    m_EncounterTiers[tier].actions.push_back(action);
    m_EncounterTiers[tier].currentAction = 0;
}

void Encounter::Update(float delta)
{
    if (m_pCarrier.expired())
    {
        return;
    }

    m_TimeToNextAction -= delta;
    if (m_TimeToNextAction < 0.0)
    {
        m_TimeToNextAction = 30.0f;

        EncounterTier& currentTier = m_EncounterTiers[m_CurrentTier];
        InstantiateAction(currentTier.actions[currentTier.currentAction]);
        currentTier.currentAction++;

        // Have we used all the actions in this tier?
        if (m_EncounterTiers[m_CurrentTier].currentAction >= m_EncounterTiers[m_CurrentTier].actions.size())
        {
            // Go to the next tier if possible
            if (m_CurrentTier + 1 < m_EncounterTiers.size() && !m_EncounterTiers[m_CurrentTier + 1].actions.empty())
            {
                m_CurrentTier++;
            }

            RebuildTier(m_CurrentTier);
        }
    }
}

void Encounter::InstantiateAction(const Encounter::EncounterAction& action)
{
    SceneWeakPtr pWeakScene = std::static_pointer_cast<Scene>(m_pSector.lock());
    for (const auto& wing : action.wings)
    {
        static WingID sWingID = 0;
        sWingID++;
        WingID currentWingID = sWingID; // So it can be captured in the lambda; temporary until this logic moves to the carrier system.

        const size_t numShips = wing.entities.size();
        for (size_t i = 0; i < numShips; i++)
        {
            const float positionZ = -15.0f * (static_cast<float>(numShips) - 1.0f) + 30.0f * static_cast<float>(i);
            EntityBuilder::Build(pWeakScene, wing.entities[i], glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, 0.0f, positionZ)), [currentWingID](EntitySharedPtr pEntity){
                pEntity->AddComponent<WingComponent>().ID = currentWingID;
                pEntity->AddComponent<FactionComponent>().Value = Faction::Hostile;
            });
        }
    }
}

} // namespace WingsOfSteel