#include <AICombat/MageStateMachine.hpp>

#include <SuperPupUtilities/Bullet.hpp>
#include <SuperPupUtilities/SimpleObjectPool.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>
#include <Canis/AudioManager.hpp>

#include <algorithm>

namespace AICombat
{
    namespace
    {
        ScriptConf mageStateMachineConf = {};
    }

    // =========================
    // Idle
    // =========================
    MageIdleState::MageIdleState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void MageIdleState::Update(float)
    {
        auto* sm = dynamic_cast<MageStateMachine*>(m_stateMachine);
        if (sm == nullptr)
            return;

        if (sm->FindClosestTarget() != nullptr)
            sm->ChangeState(MageChaseState::Name);
    }

    // =========================
    // Chase
    // =========================
    MageChaseState::MageChaseState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void MageChaseState::Update(float _dt)
    {
        auto* sm = dynamic_cast<MageStateMachine*>(m_stateMachine);
        if (sm == nullptr)
            return;

        Canis::Entity* target = sm->FindClosestTarget();

        if (target == nullptr)
        {
            sm->ChangeState(MageIdleState::Name);
            return;
        }

        sm->FaceTarget(*target);

        if (sm->DistanceTo(*target) <= sm->GetAttackRange())
        {
            sm->ChangeState(MageCastState::Name);
            return;
        }

        sm->MoveTowards(*target, moveSpeed, _dt);
    }

    // =========================
    // Cast
    // =========================
    MageCastState::MageCastState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void MageCastState::Enter()
    {
        m_hasFired = false;
    }

    void MageCastState::Update(float)
    {
        auto* sm = dynamic_cast<MageStateMachine*>(m_stateMachine);
        if (sm == nullptr)
            return;

        Canis::Entity* target = sm->FindClosestTarget();
        if (target == nullptr)
        {
            sm->ChangeState(MageIdleState::Name);
            return;
        }

        sm->FaceTarget(*target);

        // ⏱ 0.5s delay before firing
        if (!m_hasFired && sm->GetStateTime() >= castWindupTime)
        {
            sm->ShootProjectile();
            m_hasFired = true;

            if (sm->FindClosestTarget() != nullptr)
                sm->ChangeState(MageChaseState::Name);
            else
                sm->ChangeState(MageIdleState::Name);
        }
    }

    void MageCastState::Exit()
    {
        m_hasFired = false;
    }

    // =========================
    // State Machine
    // =========================
    MageStateMachine::MageStateMachine(Canis::Entity& _entity) :
        StateMachine(_entity),
        idleState(*this),
        chaseState(*this),
        castState(*this)
    {
        maxHealth = 24;
    }

    void RegisterMageStateMachineScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, targetTag);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, detectionRange);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, bodyColliderSize);

        RegisterAccessorProperty(mageStateMachineConf,
            AICombat::MageStateMachine,
            chaseState,
            moveSpeed);

        RegisterAccessorProperty(mageStateMachineConf,
            AICombat::MageStateMachine,
            castState,
            castWindupTime);

        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, attackRange);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, projectilePoolCode);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, projectileSpawnDistance);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, projectileSpawnHeight);

        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, projectileDamage);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, projectileSpeed);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, projectileLifeTime);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, projectileGravity);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, projectileDestroyOnImpact);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, projectileDestroyEntityWhenDone);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, projectileHitImpulse);

        //  Cast SFX
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, castSfx);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, castSfxVolume);

        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, maxHealth);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, logStateChanges);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, hitSfxPath1);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, hitSfxPath2);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, hitSfxVolume);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, deathEffectPrefab);

        DEFAULT_CONFIG_AND_REQUIRED(
            mageStateMachineConf,
            AICombat::MageStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        mageStateMachineConf.DEFAULT_DRAW_INSPECTOR(
            AICombat::MageStateMachine);

        _app.RegisterScript(mageStateMachineConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(mageStateMachineConf, MageStateMachine)

    void MageStateMachine::Ready()
    {
        StateMachine::Ready();

        ClearStates();
        AddState(idleState);
        AddState(chaseState);
        AddState(castState);

        ChangeState(MageIdleState::Name);
    }

    void MageStateMachine::Destroy()
    {
        StateMachine::Destroy();
    }

    float MageStateMachine::GetAttackRange() const
    {
        return attackRange;
    }

    // =========================
    // Shoot
    // =========================
    void MageStateMachine::ShootProjectile()
    {
        if (projectilePoolCode.empty())
            return;

        if (SuperPupUtilities::SimpleObjectPool::Instance == nullptr)
        {
            Canis::Debug::Warning("%s: no pool.", entity.name.c_str());
            return;
        }

        if (!entity.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& t = entity.GetComponent<Canis::Transform>();

        const Canis::Vector3 pos =
            t.GetGlobalPosition() +
            (t.GetForward() * projectileSpawnDistance) +
            Canis::Vector3(0.0f, projectileSpawnHeight, 0.0f);

        const Canis::Vector3 rot = t.GetGlobalRotation();

        Canis::Entity* proj =
            SuperPupUtilities::SimpleObjectPool::Instance->Spawn(projectilePoolCode);

        if (proj == nullptr)
            return;

        if (proj->HasComponent<Canis::Transform>())
        {
            auto& pt = proj->GetComponent<Canis::Transform>();
            pt.position = pos;
            pt.rotation = rot;
        }

        if (auto* bullet = proj->GetScript<SuperPupUtilities::Bullet>())
        {
            bullet->damage = projectileDamage;
            bullet->speed = projectileSpeed;
            bullet->lifeTime = projectileLifeTime;
            bullet->gravity = projectileGravity;
            bullet->destroyOnImpact = projectileDestroyOnImpact;
            bullet->destroyEntityWhenDone = projectileDestroyEntityWhenDone;
            bullet->hitImpulse = projectileHitImpulse;

            bullet->targetTags.clear();
            if (!targetTag.empty())
                bullet->targetTags.push_back(targetTag);

            bullet->Launch();

            //  PLAY CAST SOUND
            if (!castSfx.Empty())
            {
                Canis::AudioManager::PlaySFX(
                    castSfx,
                    std::clamp(castSfxVolume, 0.0f, 0.7f));
            }
        }
    }
}