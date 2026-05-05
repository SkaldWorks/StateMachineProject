#include <AICombat/HealerStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
#include <Canis/ConfigHelper.hpp>

#include <algorithm>
#include <limits>

namespace AICombat
{
    namespace { ScriptConf healerStateMachineConf = {}; }

    static HealerStateMachine* SM(SuperPupUtilities::StateMachine* s)
    {
        return static_cast<HealerStateMachine*>(s);
    }

    static bool InvalidTarget(HealerStateMachine* sm)
    {
        return !sm || !sm->currentHealTarget || !sm->currentHealTarget->active;
    }

    HealerIdleState::HealerIdleState(SuperPupUtilities::StateMachine& sm) :
        State(Name, sm) {}

    void HealerIdleState::Update(float)
    {
        auto* sm = SM(m_stateMachine);
        if (!sm) return;

        if (auto* t = sm->FindLowestHealthAlly())
        {
            sm->ReserveTarget(t);
            sm->ChangeState(HealerMoveState::Name);
        }
    }

    HealerMoveState::HealerMoveState(SuperPupUtilities::StateMachine& sm) :
        State(Name, sm) {}

    void HealerMoveState::Update(float dt)
    {
        auto* sm = SM(m_stateMachine);
        if (InvalidTarget(sm))
        {
            if (sm) sm->ChangeState(HealerIdleState::Name);
            return;
        }

        auto& target = *sm->currentHealTarget;
        if (!target.HasComponent<Canis::Transform>() ||
            !sm->entity.HasComponent<Canis::Transform>())
            return;

        auto& self = sm->entity.GetComponent<Canis::Transform>();

        Canis::Vector3 dir =
            sm->GetHealPosition(target, followDistance) -
            self.GetGlobalPosition();

        dir.y = 0.0f;

        if (glm::length(dir) > 0.1f)
            self.position += glm::normalize(dir) * moveSpeed * dt;

        sm->FaceTarget(target);

        if (glm::length(dir) <= 0.25f)
            sm->ChangeState(HealerHealState::Name);
    }

    HealerHealState::HealerHealState(SuperPupUtilities::StateMachine& sm) :
        State(Name, sm) {}

    void HealerHealState::Enter() { m_timer = 0.0f; }

    void HealerHealState::Update(float dt)
    {
        auto* sm = SM(m_stateMachine);
        if (InvalidTarget(sm))
        {
            if (sm) sm->ChangeState(HealerIdleState::Name);
            return;
        }

        auto* ally = sm->currentHealTarget->GetScript<StateMachine>();
        if (!ally || !ally->IsAlive() ||
            ally->GetCurrentHealth() >= ally->GetMaxHealth())
        {
            sm->ChangeState(HealerIdleState::Name);
            return;
        }

        m_timer += dt;
        const float interval = std::max(healInterval, 0.001f);

        while (m_timer >= interval)
        {
            m_timer -= interval;

            ally->Heal(healAmount);
            sm->SpawnHealEffect();

            if (!sm->healSfx.Empty())
                Canis::AudioManager::PlaySFX(
                    sm->healSfx,
                    std::clamp(sm->healSfxVolume, 0.0f, 0.3f));

            if (ally->GetCurrentHealth() >= ally->GetMaxHealth())
            {
                sm->ChangeState(HealerIdleState::Name);
                return;
            }
        }
    }

    void HealerHealState::Exit()
    {
        m_timer = 0.0f;
        if (auto* sm = SM(m_stateMachine))
            sm->ClearReservedTarget();
    }

    HealerStateMachine::HealerStateMachine(Canis::Entity& e) :
        StateMachine(e), idleState(*this), moveState(*this), healState(*this)
    {
        maxHealth = 30;
    }

    void RegisterHealerStateMachineScript(Canis::App& app)
    {
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, allyTag);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, targetTag);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, detectionRange);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, bodyColliderSize);

        RegisterAccessorProperty(healerStateMachineConf, AICombat::HealerStateMachine, moveState, moveSpeed);
        RegisterAccessorProperty(healerStateMachineConf, AICombat::HealerStateMachine, moveState, followDistance);
        RegisterAccessorProperty(healerStateMachineConf, AICombat::HealerStateMachine, healState, healAmount);
        RegisterAccessorProperty(healerStateMachineConf, AICombat::HealerStateMachine, healState, healInterval);

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
            Canis::Transform, Canis::Material, Canis::Model,
            Canis::Rigidbody, Canis::BoxCollider);

        healerStateMachineConf.DEFAULT_DRAW_INSPECTOR(AICombat::HealerStateMachine);
        app.RegisterScript(healerStateMachineConf);
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

    void HealerStateMachine::Update(float dt)
    {
        UpdateHealEffects(dt);
        StateMachine::Update(dt);
    }

    Canis::Entity* HealerStateMachine::FindLowestHealthAlly() const
    {
        Canis::Entity* best = nullptr;
        float bestRatio = std::numeric_limits<float>::max();

        for (auto* e : entity.scene.GetEntitiesWithTag(allyTag))
        {
            if (!e || e == &entity || !e->active) continue;

            auto* ally = e->GetScript<StateMachine>();
            if (!ally || !ally->IsAlive()) continue;
            if (ally->GetCurrentHealth() >= ally->GetMaxHealth()) continue;
            if (IsTargetBeingHealed(*e)) continue;

            float ratio = (float)ally->GetCurrentHealth() / ally->GetMaxHealth();
            if (ratio < bestRatio) bestRatio = ratio, best = e;
        }

        return best;
    }

    bool HealerStateMachine::IsTargetBeingHealed(Canis::Entity& t) const
    {
        for (auto* h : entity.scene.GetEntitiesWithTag(entity.tag))
        {
            auto* hs = h ? h->GetScript<HealerStateMachine>() : nullptr;
            if (hs && hs->currentHealTarget == &t) return true;
        }
        return false;
    }

    Canis::Vector3 HealerStateMachine::GetHealPosition(
        const Canis::Entity& t, float d) const
    {
        auto& tr = t.GetComponent<Canis::Transform>();
        return tr.GetGlobalPosition() - (tr.GetForward() * d);
    }

    void HealerStateMachine::ReserveTarget(Canis::Entity* t) { currentHealTarget = t; }
    void HealerStateMachine::ClearReservedTarget() { currentHealTarget = nullptr; }

    void HealerStateMachine::SpawnHealEffect()
    {
        if (healEffectPrefab.Empty() || !currentHealTarget ||
            !currentHealTarget->HasComponent<Canis::Transform>())
            return;

        auto& t = currentHealTarget->GetComponent<Canis::Transform>();

        for (auto* e : entity.scene.Instantiate(healEffectPrefab))
        {
            if (!e) continue;

            if (e->HasComponent<Canis::Transform>())
            {
                auto& et = e->GetComponent<Canis::Transform>();
                et.position = t.GetGlobalPosition();
                et.rotation = t.GetGlobalRotation();
            }

            m_pendingHealEffects.push_back({ e, healEffectLifetime });
        }
    }

    void HealerStateMachine::UpdateHealEffects(float dt)
    {
        for (size_t i = 0; i < m_pendingHealEffects.size();)
        {
            auto& e = m_pendingHealEffects[i];
            if (!e.entity || !e.entity->active || (e.timeRemaining -= dt) <= 0.0f)
            {
                if (e.entity && e.entity->active) e.entity->Destroy();
                m_pendingHealEffects.erase(m_pendingHealEffects.begin() + i);
            }
            else ++i;
        }
    }

    void HealerStateMachine::DestroyAllPendingHealEffects()
    {
        for (auto& e : m_pendingHealEffects)
            if (e.entity && e.entity->active) e.entity->Destroy();

        m_pendingHealEffects.clear();
    }
}