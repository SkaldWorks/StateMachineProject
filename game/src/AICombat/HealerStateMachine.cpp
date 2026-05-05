#include <AICombat/HealerStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>

#include <algorithm>
#include <limits>

namespace AICombat
{
    namespace
    {
        ScriptConf healerStateMachineConf = {};
    }

    HealerIdleState::HealerIdleState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HealerIdleState::Update(float)
    {
        HealerStateMachine* stateMachine =
            dynamic_cast<HealerStateMachine*>(m_stateMachine);

        if (stateMachine == nullptr)
            return;

        Canis::Entity* target = stateMachine->FindLowestHealthAlly();

        if (target == nullptr)
            return;

        stateMachine->ReserveTarget(target);
        stateMachine->ChangeState(HealerMoveState::Name);
    }

    HealerMoveState::HealerMoveState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HealerMoveState::Update(float _dt)
    {
        HealerStateMachine* stateMachine =
            dynamic_cast<HealerStateMachine*>(m_stateMachine);

        if (stateMachine == nullptr ||
            stateMachine->currentHealTarget == nullptr ||
            !stateMachine->currentHealTarget->active)
        {
            if (stateMachine != nullptr)
                stateMachine->ChangeState(HealerIdleState::Name);

            return;
        }

        Canis::Entity& target = *stateMachine->currentHealTarget;

        if (!target.HasComponent<Canis::Transform>() ||
            !stateMachine->entity.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& selfTransform =
            stateMachine->entity.GetComponent<Canis::Transform>();

        const Canis::Vector3 desiredPosition =
            stateMachine->GetHealPosition(target, followDistance);

        Canis::Vector3 moveDirection =
            desiredPosition - selfTransform.GetGlobalPosition();

        moveDirection.y = 0.0f;

        if (glm::length(moveDirection) > 0.1f)
        {
            moveDirection = glm::normalize(moveDirection);
            selfTransform.position += moveDirection * moveSpeed * _dt;
        }

        stateMachine->FaceTarget(target);

        if (glm::distance(
            selfTransform.GetGlobalPosition(),
            desiredPosition) <= 0.25f)
        {
            stateMachine->ChangeState(HealerHealState::Name);
        }
    }

    HealerHealState::HealerHealState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HealerHealState::Enter()
    {
        m_timer = 0.0f;
    }

    void HealerHealState::Update(float _dt)
    {
        HealerStateMachine* stateMachine =
            dynamic_cast<HealerStateMachine*>(m_stateMachine);

        if (stateMachine == nullptr ||
            stateMachine->currentHealTarget == nullptr ||
            !stateMachine->currentHealTarget->active)
        {
            if (stateMachine != nullptr)
                stateMachine->ChangeState(HealerIdleState::Name);

            return;
        }

        StateMachine* allyStateMachine =
            stateMachine->currentHealTarget->GetScript<StateMachine>();

        if (allyStateMachine == nullptr || !allyStateMachine->IsAlive())
        {
            stateMachine->ChangeState(HealerIdleState::Name);
            return;
        }

        if (allyStateMachine->GetCurrentHealth() >=
            allyStateMachine->GetMaxHealth())
        {
            stateMachine->ChangeState(HealerIdleState::Name);
            return;
        }

        m_timer += _dt;

        const float interval = std::max(healInterval, 0.001f);

        while (m_timer >= interval)
        {
            m_timer -= interval;

            if (stateMachine == nullptr ||
                stateMachine->currentHealTarget == nullptr ||
                !stateMachine->currentHealTarget->active)
                return;

            allyStateMachine =
                stateMachine->currentHealTarget->GetScript<StateMachine>();

            if (allyStateMachine == nullptr || !allyStateMachine->IsAlive())
            {
                stateMachine->ChangeState(HealerIdleState::Name);
                return;
            }

            if (allyStateMachine->GetCurrentHealth() >=
                allyStateMachine->GetMaxHealth())
            {
                stateMachine->ChangeState(HealerIdleState::Name);
                return;
            }

            allyStateMachine->Heal(healAmount);
            stateMachine->SpawnHealEffect();

            if (!stateMachine->healSfx.Empty())
            {
                Canis::AudioManager::PlaySFX(
                    stateMachine->healSfx,
                    std::clamp(stateMachine->healSfxVolume, 0.0f, 0.3f));
            }

            if (allyStateMachine->GetCurrentHealth() >=
                allyStateMachine->GetMaxHealth())
            {
                stateMachine->ChangeState(HealerIdleState::Name);
                return;
            }
        }
    }

    void HealerHealState::Exit()
    {
        m_timer = 0.0f;

        if (HealerStateMachine* stateMachine =
            dynamic_cast<HealerStateMachine*>(m_stateMachine))
        {
            stateMachine->ClearReservedTarget();
        }
    }

    HealerStateMachine::HealerStateMachine(Canis::Entity& _entity) :
        StateMachine(_entity),
        idleState(*this),
        moveState(*this),
        healState(*this)
    {
        maxHealth = 30;
    }

    void RegisterHealerStateMachineScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, allyTag);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, targetTag);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, detectionRange);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, bodyColliderSize);

        RegisterAccessorProperty(healerStateMachineConf,
            AICombat::HealerStateMachine,
            moveState,
            moveSpeed);

        RegisterAccessorProperty(healerStateMachineConf,
            AICombat::HealerStateMachine,
            moveState,
            followDistance);

        RegisterAccessorProperty(healerStateMachineConf,
            AICombat::HealerStateMachine,
            healState,
            healAmount);

        RegisterAccessorProperty(healerStateMachineConf,
            AICombat::HealerStateMachine,
            healState,
            healInterval);

        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, healSfx);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, healSfxVolume);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, healEffectPrefab);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, healEffectLifetime);

        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, maxHealth);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, logStateChanges);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, hitSfxPath1);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, hitSfxPath2);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, hitSfxVolume);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, deathEffectPrefab);

        DEFAULT_CONFIG_AND_REQUIRED(
            healerStateMachineConf,
            AICombat::HealerStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        healerStateMachineConf.DEFAULT_DRAW_INSPECTOR(
            AICombat::HealerStateMachine);

        _app.RegisterScript(healerStateMachineConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(healerStateMachineConf, HealerStateMachine)

    void HealerStateMachine::Ready()
    {
        StateMachine::Ready();

        ClearStates();
        AddState(idleState);
        AddState(moveState);
        AddState(healState);

        ChangeState(HealerIdleState::Name);
    }

    void HealerStateMachine::Destroy()
    {
        DestroyAllPendingHealEffects();
        currentHealTarget = nullptr;
        StateMachine::Destroy();
    }

    void HealerStateMachine::Update(float _dt)
    {
        UpdateHealEffects(_dt);
        StateMachine::Update(_dt);
    }

    Canis::Entity* HealerStateMachine::FindLowestHealthAlly() const
    {
        Canis::Entity* bestTarget = nullptr;
        float lowestHealthRatio = std::numeric_limits<float>::max();

        for (Canis::Entity* candidate :
            entity.scene.GetEntitiesWithTag(allyTag))
        {
            if (candidate == nullptr ||
                candidate == &entity ||
                !candidate->active)
                continue;

            StateMachine* ally =
                candidate->GetScript<StateMachine>();

            if (ally == nullptr ||
                !ally->IsAlive())
                continue;

            if (ally->GetCurrentHealth() >= ally->GetMaxHealth())
                continue;

            if (IsTargetBeingHealed(*candidate))
                continue;

            const float healthRatio =
                static_cast<float>(ally->GetCurrentHealth()) /
                static_cast<float>(ally->GetMaxHealth());

            if (healthRatio < lowestHealthRatio)
            {
                lowestHealthRatio = healthRatio;
                bestTarget = candidate;
            }
        }

        return bestTarget;
    }

    bool HealerStateMachine::IsTargetBeingHealed(Canis::Entity& _target) const
    {
        for (Canis::Entity* healer :
            entity.scene.GetEntitiesWithTag(entity.tag))
        {
            if (healer == nullptr ||
                healer == &entity ||
                !healer->active)
                continue;

            HealerStateMachine* healerScript =
                healer->GetScript<HealerStateMachine>();

            if (healerScript == nullptr)
                continue;

            if (healerScript->currentHealTarget == &_target)
                return true;
        }

        return false;
    }

    Canis::Vector3 HealerStateMachine::GetHealPosition(
        const Canis::Entity& _target,
        float _distance) const
    {
        const Canis::Transform& targetTransform =
            _target.GetComponent<Canis::Transform>();

        return targetTransform.GetGlobalPosition() -
            (targetTransform.GetForward() * _distance);
    }

    void HealerStateMachine::ReserveTarget(Canis::Entity* _target)
    {
        currentHealTarget = _target;
    }

    void HealerStateMachine::ClearReservedTarget()
    {
        currentHealTarget = nullptr;
    }

    void HealerStateMachine::SpawnHealEffect()
    {
        if (healEffectPrefab.Empty() || currentHealTarget == nullptr)
            return;

        if (!currentHealTarget->active)
            return;

        if (!currentHealTarget->HasComponent<Canis::Transform>())
            return;

        const Canis::Transform& targetTransform =
            currentHealTarget->GetComponent<Canis::Transform>();

        const Canis::Vector3 spawnPosition =
            targetTransform.GetGlobalPosition();

        const Canis::Vector3 spawnRotation =
            targetTransform.GetGlobalRotation();

        std::vector<Canis::Entity*> spawnedEntities =
            entity.scene.Instantiate(healEffectPrefab);

        const float lifetime = std::max(healEffectLifetime, 0.0f);

        for (Canis::Entity* spawnedEntity : spawnedEntities)
        {
            if (spawnedEntity == nullptr)
                continue;

            if (spawnedEntity->HasComponent<Canis::Transform>())
            {
                Canis::Transform& spawnedTransform =
                    spawnedEntity->GetComponent<Canis::Transform>();

                spawnedTransform.position = spawnPosition;
                spawnedTransform.rotation = spawnRotation;
            }

            m_pendingHealEffects.push_back(PendingHealEffect
            {
                .entity = spawnedEntity,
                .timeRemaining = lifetime
            });
        }
    }

    void HealerStateMachine::UpdateHealEffects(float _dt)
    {
        for (size_t i = 0; i < m_pendingHealEffects.size();)
        {
            PendingHealEffect& effect = m_pendingHealEffects[i];

            if (effect.entity == nullptr)
            {
                m_pendingHealEffects.erase(m_pendingHealEffects.begin() + i);
                continue;
            }

            if (!effect.entity->active)
            {
                m_pendingHealEffects.erase(m_pendingHealEffects.begin() + i);
                continue;
            }

            effect.timeRemaining -= _dt;

            if (effect.timeRemaining > 0.0f)
            {
                ++i;
                continue;
            }

            effect.entity->Destroy();
            m_pendingHealEffects.erase(m_pendingHealEffects.begin() + i);
        }
    }

    void HealerStateMachine::DestroyAllPendingHealEffects()
    {
        for (PendingHealEffect& effect : m_pendingHealEffects)
        {
            if (effect.entity != nullptr && effect.entity->active)
                effect.entity->Destroy();
        }

        m_pendingHealEffects.clear();
    }
}