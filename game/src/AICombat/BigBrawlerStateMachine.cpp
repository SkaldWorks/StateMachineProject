#include <AICombat/BigBrawlerStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>

#include <algorithm>

namespace AICombat
{
    namespace { ScriptConf conf = {}; }

    static BigBrawlerStateMachine* SM(SuperPupUtilities::StateMachine* s)
    {
        return dynamic_cast<BigBrawlerStateMachine*>(s);
    }

    BigIdleState::BigIdleState(SuperPupUtilities::StateMachine& s) : State(Name, s) {}

    void BigIdleState::Enter()
    {
        if (auto* sm = SM(m_stateMachine)) sm->ResetHammerPose();
    }

    void BigIdleState::Update(float)
    {
        if (auto* sm = SM(m_stateMachine))
            if (sm->FindClosestTarget())
                sm->ChangeState(BigChaseState::Name);
    }

    BigChaseState::BigChaseState(SuperPupUtilities::StateMachine& s) : State(Name, s) {}

    void BigChaseState::Enter()
    {
        if (auto* sm = SM(m_stateMachine)) sm->ResetHammerPose();
    }

    void BigChaseState::Update(float dt)
    {
        auto* sm = SM(m_stateMachine);
        if (!sm) return;

        auto* t = sm->FindClosestTarget();
        if (!t) return sm->ChangeState(BigIdleState::Name);

        sm->FaceTarget(*t);

        if (sm->DistanceTo(*t) <= sm->GetAttackRange())
            return sm->ChangeState(BigHammerTimeState::Name);

        sm->MoveTowards(*t, moveSpeed, dt);
    }

    BigHammerTimeState::BigHammerTimeState(SuperPupUtilities::StateMachine& s) : State(Name, s) {}

    void BigHammerTimeState::Enter()
    {
        if (auto* sm = SM(m_stateMachine)) sm->SetHammerSwing(0.0f);
    }

    void BigHammerTimeState::Update(float)
    {
        auto* sm = SM(m_stateMachine);
        if (!sm) return;

        if (auto* t = sm->FindClosestTarget())
            sm->FaceTarget(*t);

        const float dur = std::max(attackDuration, 0.001f);
        sm->SetHammerSwing(sm->GetStateTime() / dur);

        if (sm->GetStateTime() < dur) return;

        sm->ChangeState(sm->FindClosestTarget()
            ? BigChaseState::Name
            : BigIdleState::Name);
    }

    void BigHammerTimeState::Exit()
    {
        if (auto* sm = SM(m_stateMachine)) sm->ResetHammerPose();
    }

    BigBrawlerStateMachine::BigBrawlerStateMachine(Canis::Entity& e) :
        StateMachine(e), idleState(*this), chaseState(*this), hammerTimeState(*this)
    {
        maxHealth = 80;
    }

    void RegisterBigBrawlerStateMachineScript(Canis::App& app)
    {
        REGISTER_PROPERTY(conf, AICombat::BigBrawlerStateMachine, targetTag);
        REGISTER_PROPERTY(conf, AICombat::BigBrawlerStateMachine, detectionRange);
        REGISTER_PROPERTY(conf, AICombat::BigBrawlerStateMachine, bodyColliderSize);

        RegisterAccessorProperty(conf, AICombat::BigBrawlerStateMachine, chaseState, moveSpeed);
        RegisterAccessorProperty(conf, AICombat::BigBrawlerStateMachine, hammerTimeState, hammerRestDegrees);
        RegisterAccessorProperty(conf, AICombat::BigBrawlerStateMachine, hammerTimeState, hammerSwingDegrees);
        RegisterAccessorProperty(conf, AICombat::BigBrawlerStateMachine, hammerTimeState, attackRange);
        RegisterAccessorProperty(conf, AICombat::BigBrawlerStateMachine, hammerTimeState, attackDuration);
        RegisterAccessorProperty(conf, AICombat::BigBrawlerStateMachine, hammerTimeState, attackDamageTime);

        REGISTER_PROPERTY(conf, AICombat::BigBrawlerStateMachine, maxHealth);
        REGISTER_PROPERTY(conf, AICombat::BigBrawlerStateMachine, logStateChanges);
        REGISTER_PROPERTY(conf, AICombat::BigBrawlerStateMachine, hammerVisual);
        REGISTER_PROPERTY(conf, AICombat::BigBrawlerStateMachine, hitSfxPath1);
        REGISTER_PROPERTY(conf, AICombat::BigBrawlerStateMachine, hitSfxPath2);
        REGISTER_PROPERTY(conf, AICombat::BigBrawlerStateMachine, hitSfxVolume);
        REGISTER_PROPERTY(conf, AICombat::BigBrawlerStateMachine, deathEffectPrefab);

        DEFAULT_CONFIG_AND_REQUIRED(
            conf,
            AICombat::BigBrawlerStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        conf.DEFAULT_DRAW_INSPECTOR(AICombat::BigBrawlerStateMachine);
        app.RegisterScript(conf);
    }

    DEFAULT_UNREGISTER_SCRIPT(conf, BigBrawlerStateMachine)

    void BigBrawlerStateMachine::Ready()
    {
        StateMachine::Ready();
        ClearStates();
        AddState(idleState);
        AddState(chaseState);
        AddState(hammerTimeState);
        ResetHammerPose();
        ChangeState(BigIdleState::Name);
    }

    void BigBrawlerStateMachine::Destroy()
    {
        hammerVisual = nullptr;
        StateMachine::Destroy();
    }

    float BigBrawlerStateMachine::GetAttackRange() const
    {
        return hammerTimeState.attackRange;
    }

    void BigBrawlerStateMachine::ResetHammerPose()
    {
        SetHammerSwing(0.0f);
    }

    void BigBrawlerStateMachine::SetHammerSwing(float n)
    {
        if (!hammerVisual || !hammerVisual->HasComponent<Canis::Transform>())
            return;

        auto& t = hammerVisual->GetComponent<Canis::Transform>();
        n = Clamp01(n);

        const float blend = (n <= 0.5f) ? n * 2.0f : (1.0f - n) * 2.0f;

        t.rotation.x = DEG2RAD *
            (hammerTimeState.hammerRestDegrees +
             hammerTimeState.hammerSwingDegrees * blend);
    }
}