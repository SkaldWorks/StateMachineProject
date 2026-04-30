#include <AICombat/BigBrawlerStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>

#include <algorithm>

namespace AICombat
{
    namespace
    {
        ScriptConf bigBrawlerStateMachineConf = {};
    }

    BigIdleState::BigIdleState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void BigIdleState::Enter()
    {
        if (BigBrawlerStateMachine* stateMachine =
            dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine))
        {
            stateMachine->ResetHammerPose();
        }
    }

    void BigIdleState::Update(float)
    {
        if (BigBrawlerStateMachine* stateMachine =
            dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine))
        {
            if (stateMachine->FindClosestTarget() != nullptr)
                stateMachine->ChangeState(BigChaseState::Name);
        }
    }

    BigChaseState::BigChaseState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void BigChaseState::Enter()
    {
        if (BigBrawlerStateMachine* stateMachine =
            dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine))
        {
            stateMachine->ResetHammerPose();
        }
    }

    void BigChaseState::Update(float _dt)
    {
        BigBrawlerStateMachine* stateMachine =
            dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine);

        if (stateMachine == nullptr)
            return;

        Canis::Entity* target = stateMachine->FindClosestTarget();

        if (target == nullptr)
        {
            stateMachine->ChangeState(BigIdleState::Name);
            return;
        }

        stateMachine->FaceTarget(*target);

        if (stateMachine->DistanceTo(*target) <= stateMachine->GetAttackRange())
        {
            stateMachine->ChangeState(BigHammerTimeState::Name);
            return;
        }

        stateMachine->MoveTowards(*target, moveSpeed, _dt);
    }

    BigHammerTimeState::BigHammerTimeState(
        SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void BigHammerTimeState::Enter()
    {
        if (BigBrawlerStateMachine* stateMachine =
            dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine))
        {
            stateMachine->SetHammerSwing(0.0f);
        }
    }

    void BigHammerTimeState::Update(float)
    {
        BigBrawlerStateMachine* stateMachine =
            dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine);

        if (stateMachine == nullptr)
            return;

        if (Canis::Entity* target = stateMachine->FindClosestTarget())
            stateMachine->FaceTarget(*target);

        const float duration = std::max(attackDuration, 0.001f);

        stateMachine->SetHammerSwing(
            stateMachine->GetStateTime() / duration);

        if (stateMachine->GetStateTime() < duration)
            return;

        if (stateMachine->FindClosestTarget() != nullptr)
            stateMachine->ChangeState(BigChaseState::Name);
        else
            stateMachine->ChangeState(BigIdleState::Name);
    }

    void BigHammerTimeState::Exit()
    {
        if (BigBrawlerStateMachine* stateMachine =
            dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine))
        {
            stateMachine->ResetHammerPose();
        }
    }

    BigBrawlerStateMachine::BigBrawlerStateMachine(Canis::Entity& _entity) :
        StateMachine(_entity),
        idleState(*this),
        chaseState(*this),
        hammerTimeState(*this)
    {
        maxHealth = 80;
    }

    void RegisterBigBrawlerStateMachineScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, targetTag);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, detectionRange);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, bodyColliderSize);

        RegisterAccessorProperty(bigBrawlerStateMachineConf,
            AICombat::BigBrawlerStateMachine,
            chaseState,
            moveSpeed);

        RegisterAccessorProperty(bigBrawlerStateMachineConf,
            AICombat::BigBrawlerStateMachine,
            hammerTimeState,
            hammerRestDegrees);

        RegisterAccessorProperty(bigBrawlerStateMachineConf,
            AICombat::BigBrawlerStateMachine,
            hammerTimeState,
            hammerSwingDegrees);

        RegisterAccessorProperty(bigBrawlerStateMachineConf,
            AICombat::BigBrawlerStateMachine,
            hammerTimeState,
            attackRange);

        RegisterAccessorProperty(bigBrawlerStateMachineConf,
            AICombat::BigBrawlerStateMachine,
            hammerTimeState,
            attackDuration);

        RegisterAccessorProperty(bigBrawlerStateMachineConf,
            AICombat::BigBrawlerStateMachine,
            hammerTimeState,
            attackDamageTime);

        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, maxHealth);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, logStateChanges);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hammerVisual);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hitSfxPath1);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hitSfxPath2);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hitSfxVolume);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, deathEffectPrefab);

        DEFAULT_CONFIG_AND_REQUIRED(
            bigBrawlerStateMachineConf,
            AICombat::BigBrawlerStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        bigBrawlerStateMachineConf.DEFAULT_DRAW_INSPECTOR(
            AICombat::BigBrawlerStateMachine);

        _app.RegisterScript(bigBrawlerStateMachineConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(bigBrawlerStateMachineConf, BigBrawlerStateMachine)

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

    void BigBrawlerStateMachine::SetHammerSwing(float _normalized)
    {
        if (hammerVisual == nullptr ||
            !hammerVisual->HasComponent<Canis::Transform>())
            return;

        Canis::Transform& hammerTransform =
            hammerVisual->GetComponent<Canis::Transform>();

        const float normalized = Clamp01(_normalized);

        const float swingBlend =
            (normalized <= 0.5f)
            ? normalized * 2.0f
            : (1.0f - normalized) * 2.0f;

        hammerTransform.rotation.x =
            DEG2RAD *
            (hammerTimeState.hammerRestDegrees +
                (hammerTimeState.hammerSwingDegrees * swingBlend));
    }
}