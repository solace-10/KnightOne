#include <cassert>

#include <glm/gtc/type_ptr.hpp>

#include "pandora.hpp"
#include "physics/collision_shape.hpp"
#include "resources/resource_system.hpp"
#include "scene/components/rigid_body_component.hpp"

namespace WingsOfSteel::Pandora
{

void RigidBodyComponent::Deserialize(const nlohmann::json& json)
{
    m_MotionType = DeserializeEnum<MotionType>(json, "motionType", MotionType::Dynamic);
    m_Mass = DeserializeRequired<int32_t>(json, "mass");
    m_LinearDamping = DeserializeRequired<float>(json, "linearDamping");
    m_AngularDamping = DeserializeRequired<float>(json, "angularDamping");
    m_CentreOfMass = DeserializeVec3(json, "centreOfMass");
    m_LinearFactor = DeserializeVec3(json, "linearFactor");
    m_AngularFactor = DeserializeVec3(json, "angularFactor");
    

    assert((m_Mass > 0 && m_MotionType == MotionType::Dynamic) || (m_Mass == 0 && m_MotionType == MotionType::Static));

    m_ResourcePath = DeserializeRequired<std::string>(json, "resource");
    GetResourceSystem()->RequestResource(m_ResourcePath, [this](ResourceSharedPtr pResource) {
        m_pResource = std::dynamic_pointer_cast<ResourceModel>(pResource);

        btTransform worldTransform;
        worldTransform.setIdentity();
        //worldTransform.setFromOpenGLMatrix(glm::value_ptr(ci.GetWorldTransform()));
        m_pMotionState = std::make_unique<btDefaultMotionState>(worldTransform);

        m_pShape = m_pResource->GetCollisionShape();
        btCollisionShape* pCollisionShape = m_pShape->GetBulletShape();
        btVector3 localInertia(0.0f, 0.0f, 0.0f);
        if (m_MotionType == MotionType::Dynamic)
        {
            pCollisionShape->calculateLocalInertia(static_cast<btScalar>(m_Mass), localInertia);
        }

        btRigidBody::btRigidBodyConstructionInfo rbInfo(
            static_cast<btScalar>(m_Mass),
            m_pMotionState.get(),
            m_pShape->GetBulletShape(),
            localInertia);

        rbInfo.m_linearDamping = m_LinearDamping;
        rbInfo.m_angularDamping = m_AngularDamping;

        m_pRigidBody = std::make_unique<btRigidBody>(rbInfo);
        m_pRigidBody->setActivationState(DISABLE_DEACTIVATION);
        m_pRigidBody->setCollisionFlags(m_pRigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
        m_pRigidBody->setUserPointer(this);

        CalculateInvInertiaTensorWorld();
    });
}

glm::mat4x4 RigidBodyComponent::GetWorldTransform() const
{
    if (m_pMotionState)
    {
        btTransform tr;
        m_pMotionState->getWorldTransform(tr);

        float mat[16];
        tr.getOpenGLMatrix(mat);

        return glm::make_mat4x4(mat);
    }
    else
    {
        return glm::mat4(1.0f);
    }
}

glm::vec3 RigidBodyComponent::GetPosition() const
{
    if (m_pMotionState)
    {
        btTransform tr;
        m_pMotionState->getWorldTransform(tr);
        const btVector3& position = tr.getOrigin();
        return glm::vec3(position.x(), position.y(), position.z());
    }
    else
    {
        return glm::vec3(0.0f);
    }
}

glm::vec3 RigidBodyComponent::GetLinearVelocity() const
{
    if (m_pRigidBody)
    {
        const btVector3& linearVelocity = m_pRigidBody->getLinearVelocity();
        return glm::vec3(linearVelocity.x(), linearVelocity.y(), linearVelocity.z());
    }
    else
    {
        return glm::vec3(0.0f);
    }
}

glm::vec3 RigidBodyComponent::GetAngularVelocity() const
{
    if (m_pRigidBody)
    {
        const btVector3& angularVelocity = m_pRigidBody->getAngularVelocity();
        return glm::vec3(angularVelocity.x(), angularVelocity.y(), angularVelocity.z());
    }
    else
    {
        return glm::vec3(0.0f);
    }
}

void RigidBodyComponent::SetLinearDamping(float value)
{
    m_LinearDamping = value;
    m_pRigidBody->setDamping(m_LinearDamping, m_AngularDamping);
}

void RigidBodyComponent::SetAngularDamping(float value)
{
    m_AngularDamping = value;
    m_pRigidBody->setDamping(m_LinearDamping, m_AngularDamping);
}

void RigidBodyComponent::SetWorldTransform(const glm::mat4x4& worldTransform)
{
    btTransform tr;
    tr.setFromOpenGLMatrix(glm::value_ptr(worldTransform));
    m_pRigidBody->setWorldTransform(tr);
    m_pMotionState->setWorldTransform(tr);
    m_pRigidBody->clearForces();
}

void RigidBodyComponent::SetLinearVelocity(const glm::vec3& linearVelocity)
{
    m_pRigidBody->setLinearVelocity(btVector3(linearVelocity.x, linearVelocity.y, linearVelocity.z));
}

void RigidBodyComponent::SetAngularVelocity(const glm::vec3& angularVelocity)
{
    m_pRigidBody->setAngularVelocity(btVector3(angularVelocity.x, angularVelocity.y, angularVelocity.z));
}

void RigidBodyComponent::SetMotionType(MotionType motionType)
{
    int flags = m_pRigidBody->getCollisionFlags();
    if (motionType == MotionType::Static)
    {
        flags |= btCollisionObject::CF_STATIC_OBJECT;
    }
    else
    {
        flags &= btCollisionObject::CF_STATIC_OBJECT;
    }
    m_pRigidBody->setCollisionFlags(flags);

    m_MotionType = motionType;
}

void RigidBodyComponent::ApplyAngularForce(const glm::vec3& force)
{
    m_pRigidBody->applyTorque(btVector3(force.x, force.y, force.z));
}

void RigidBodyComponent::ApplyLinearForce(const glm::vec3& force)
{
    m_pRigidBody->applyCentralForce(btVector3(force.x, force.y, force.z));
}

void RigidBodyComponent::SetLinearFactor(const glm::vec3& linearFactor)
{
    m_pRigidBody->setLinearFactor(btVector3(linearFactor.x, linearFactor.y, linearFactor.z));
}

void RigidBodyComponent::SetAngularFactor(const glm::vec3& angularFactor)
{
    m_pRigidBody->setAngularFactor(btVector3(angularFactor.x, angularFactor.y, angularFactor.z));
}

const glm::vec3 RigidBodyComponent::GetForwardVector() const
{
    const btVector3 dir = m_pRigidBody->getWorldTransform().getBasis()[0];
    return -glm::vec3(dir[2], dir[1], dir[0]);
}

const glm::vec3 RigidBodyComponent::GetUpVector() const
{
    const btVector3 dir = m_pRigidBody->getWorldTransform().getBasis()[1];
    return glm::vec3(dir[2], dir[1], dir[0]);
}

const glm::vec3 RigidBodyComponent::GetRightVector() const
{
    const btVector3 dir = m_pRigidBody->getWorldTransform().getBasis()[2];
    return glm::vec3(dir[2], dir[1], dir[0]);
}

void RigidBodyComponent::CalculateInvInertiaTensorWorld()
{
    // We need to transpose Bullet's matrix as bullet uses row major matrices while OpenGL uses
    // column major (note the [j][i] = [i][j]) but in practice this makes no difference for
    // this matrix, as it only has values on the diagonals.
    const btMatrix3x3& invInertiaTensorWorld = m_pRigidBody->getInvInertiaTensorWorld();
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            m_InvInertiaTensorWorld[j][i] = invInertiaTensorWorld[i][j];
        }
    }
}

} // namespace WingsOfSteel::Pandora