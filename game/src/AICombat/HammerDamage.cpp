#include <AICombat/HammerDamage.hpp>

#include <AICombat/StateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>

#include <algorithm>

namespace AICombat
{
    namespace
    {
        ScriptConf hammerDamageConf = {};
    }

    void RegisterHammerDamageScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(hammerDamageConf, AICombat::HammerDamage, owner);
        REGISTER_PROPERTY(hammerDamageConf, AICombat::HammerDamage, sensorSize);
        REGISTER_PROPERTY(hammerDamageConf, AICombat::HammerDamage, damage);
        REGISTER_PROPERTY(hammerDamageConf, AICombat::HammerDamage, targetTag);

        DEFAULT_CONFIG_AND_REQUIRED(
            hammerDamageConf,
            AICombat::HammerDamage,
            Canis::Transform,
            Canis::Rigidbody,
            Canis::BoxCollider);

        hammerDamageConf.DEFAULT_DRAW_INSPECTOR(AICombat::HammerDamage);

        _app.RegisterScript(hammerDamageConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(hammerDamageConf, HammerDamage)

    void HammerDamage::Create()
    {
        entity.GetComponent<Canis::Transform>();

        Canis::Rigidbody& rigidbody = entity.GetComponent<Canis::Rigidbody>();
        rigidbody.motionType = Canis::RigidbodyMotionType::STATIC;
        rigidbody.useGravity = false;
        rigidbody.isSensor = true;
        rigidbody.allowSleeping = false;
        rigidbody.linearVelocity = Canis::Vector3(0.0f);
        rigidbody.angularVelocity = Canis::Vector3(0.0f);

        entity.GetComponent<Canis::BoxCollider>().size = sensorSize;
    }

    void HammerDamage::Ready()
    {
        if (owner == nullptr)
            owner = FindOwnerFromHierarchy();

        if (targetTag.empty())
        {
            if (StateMachine* ownerStateMachine = GetOwnerStateMachine())
                targetTag = ownerStateMachine->targetTag;
        }
    }

    void HammerDamage::Update(float)
    {
        CheckSensorEnter();
    }

    void HammerDamage::CheckSensorEnter()
    {
        if (!entity.HasComponents<Canis::BoxCollider, Canis::Rigidbody>())
            return;

        StateMachine* ownerStateMachine = GetOwnerStateMachine();

        if (ownerStateMachine == nullptr || !ownerStateMachine->IsAlive())
        {
            m_hitTargetsThisSwing.clear();
            return;
        }

        const std::string currentState = ownerStateMachine->GetCurrentStateName();

        const bool damageWindowOpen =
            (currentState == "HammerTimeState" ||
             currentState == "BigHammerTimeState") &&
            ownerStateMachine->GetStateTime() >= 0.25f;

        if (!damageWindowOpen)
        {
            m_hitTargetsThisSwing.clear();
            return;
        }

        for (Canis::Entity* other :
            entity.GetComponent<Canis::BoxCollider>().entered)
        {
            if (other == nullptr ||
                !other->active ||
                other == owner ||
                HasDamagedThisSwing(*other))
            {
                continue;
            }

            StateMachine* targetStateMachine =
                other->GetScript<StateMachine>();

            if (targetStateMachine == nullptr ||
                !targetStateMachine->IsAlive())
            {
                continue;
            }

            if (other->tag != targetTag)
                continue;

            targetStateMachine->TakeDamage(damage);
            m_hitTargetsThisSwing.push_back(other);
        }
    }

    StateMachine* HammerDamage::GetOwnerStateMachine()
    {
        if (owner == nullptr)
            owner = FindOwnerFromHierarchy();

        if (owner == nullptr || !owner->active)
            return nullptr;

        return owner->GetScript<StateMachine>();
    }

    Canis::Entity* HammerDamage::FindOwnerFromHierarchy() const
    {
        if (!entity.HasComponent<Canis::Transform>())
            return nullptr;

        Canis::Entity* current =
            entity.GetComponent<Canis::Transform>().parent;

        while (current != nullptr)
        {
            if (current->HasScript<StateMachine>())
                return current;

            if (!current->HasComponent<Canis::Transform>())
                break;

            current =
                current->GetComponent<Canis::Transform>().parent;
        }

        return nullptr;
    }

    bool HammerDamage::HasDamagedThisSwing(Canis::Entity& _target) const
    {
        return std::find(
            m_hitTargetsThisSwing.begin(),
            m_hitTargetsThisSwing.end(),
            &_target)
            != m_hitTargetsThisSwing.end();
    }
}