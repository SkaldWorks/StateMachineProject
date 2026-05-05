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
    namespace { ScriptConf conf = {}; }

    static MageStateMachine* SM(SuperPupUtilities::StateMachine* s)
    {
        return dynamic_cast<MageStateMachine*>(s);
    }

    MageIdleState::MageIdleState(SuperPupUtilities::StateMachine& s) : State(Name, s) {}

    void MageIdleState::Update(float)
    {
        if (auto* sm = SM(m_stateMachine))
            if (sm->FindClosestTarget())
                sm->ChangeState(MageChaseState::Name);
    }

    MageChaseState::MageChaseState(SuperPupUtilities::StateMachine& s) : State(Name, s) {}

    void MageChaseState::Update(float dt)
    {
        auto* sm = SM(m_stateMachine);
        if (!sm) return;

        auto* t = sm->FindClosestTarget();
        if (!t) return sm->ChangeState(MageIdleState::Name);

        sm->FaceTarget(*t);

        if (sm->DistanceTo(*t) <= sm->GetAttackRange())
            return sm->ChangeState(MageCastState::Name);

        sm->MoveTowards(*t, moveSpeed, dt);
    }

    MageCastState::MageCastState(SuperPupUtilities::StateMachine& s) : State(Name, s) {}

    void MageCastState::Enter() { m_hasFired = false; }

    void MageCastState::Update(float)
    {
        auto* sm = SM(m_stateMachine);
        if (!sm) return;

        auto* t = sm->FindClosestTarget();
        if (!t) return sm->ChangeState(MageIdleState::Name);

        sm->FaceTarget(*t);

        if (!m_hasFired && sm->GetStateTime() >= castWindupTime)
        {
            sm->ShootProjectile();
            m_hasFired = true;
            sm->ChangeState(sm->FindClosestTarget()
                ? MageChaseState::Name
                : MageIdleState::Name);
        }
    }

    void MageCastState::Exit() { m_hasFired = false; }

    MageStateMachine::MageStateMachine(Canis::Entity& e) :
        StateMachine(e), idleState(*this), chaseState(*this), castState(*this)
    {
        maxHealth = 24;
    }

    void RegisterMageStateMachineScript(Canis::App& app)
    {
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, targetTag);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, detectionRange);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, bodyColliderSize);

        RegisterAccessorProperty(conf, AICombat::MageStateMachine, chaseState, moveSpeed);
        RegisterAccessorProperty(conf, AICombat::MageStateMachine, castState, castWindupTime);

        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, attackRange);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, projectilePoolCode);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, projectileSpawnDistance);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, projectileSpawnHeight);

        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, projectileDamage);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, projectileSpeed);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, projectileLifeTime);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, projectileGravity);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, projectileDestroyOnImpact);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, projectileDestroyEntityWhenDone);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, projectileHitImpulse);

        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, castSfx);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, castSfxVolume);

        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, maxHealth);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, logStateChanges);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, hitSfxPath1);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, hitSfxPath2);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, hitSfxVolume);
        REGISTER_PROPERTY(conf, AICombat::MageStateMachine, deathEffectPrefab);

        DEFAULT_CONFIG_AND_REQUIRED(
            conf,
            AICombat::MageStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        conf.DEFAULT_DRAW_INSPECTOR(AICombat::MageStateMachine);
        app.RegisterScript(conf);
    }

    DEFAULT_UNREGISTER_SCRIPT(conf, MageStateMachine)

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

    void MageStateMachine::ShootProjectile()
    {
        if (projectilePoolCode.empty()) return;

        if (!SuperPupUtilities::SimpleObjectPool::Instance)
        {
            Canis::Debug::Warning("%s: no pool.", entity.name.c_str());
            return;
        }

        if (!entity.HasComponent<Canis::Transform>()) return;

        auto& t = entity.GetComponent<Canis::Transform>();

        const auto pos = t.GetGlobalPosition() +
            (t.GetForward() * projectileSpawnDistance) +
            Canis::Vector3(0.0f, projectileSpawnHeight, 0.0f);

        const auto rot = t.GetGlobalRotation();

        auto* proj = SuperPupUtilities::SimpleObjectPool::Instance->Spawn(projectilePoolCode);
        if (!proj) return;

        if (proj->HasComponent<Canis::Transform>())
        {
            auto& pt = proj->GetComponent<Canis::Transform>();
            pt.position = pos;
            pt.rotation = rot;
        }

        if (auto* b = proj->GetScript<SuperPupUtilities::Bullet>())
        {
            b->damage = projectileDamage;
            b->speed = projectileSpeed;
            b->lifeTime = projectileLifeTime;
            b->gravity = projectileGravity;
            b->destroyOnImpact = projectileDestroyOnImpact;
            b->destroyEntityWhenDone = projectileDestroyEntityWhenDone;
            b->hitImpulse = projectileHitImpulse;

            b->targetTags.clear();
            if (!targetTag.empty()) b->targetTags.push_back(targetTag);

            b->Launch();

            if (!castSfx.Empty())
                Canis::AudioManager::PlaySFX(
                    castSfx,
                    std::clamp(castSfxVolume, 0.0f, 0.7f));
        }
    }
}