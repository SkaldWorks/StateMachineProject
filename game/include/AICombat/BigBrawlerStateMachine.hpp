#pragma once

#include <AICombat/StateMachine.hpp>

namespace AICombat
{
    class BigBrawlerStateMachine;

    class BigIdleState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "BigIdleState";

        explicit BigIdleState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class BigChaseState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "BigChaseState";
        float moveSpeed = 4.0f;

        explicit BigChaseState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class BigHammerTimeState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "BigHammerTimeState";
        float hammerRestDegrees = 140.0f;
        float hammerSwingDegrees = -120.0f;
        float attackRange = 2.25f;
        float attackDuration = 0.75f;
        float attackDamageTime = 0.25f;

        explicit BigHammerTimeState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
        void Exit() override;
    };

    class BigBrawlerStateMachine : public StateMachine
    {
    public:
        static constexpr const char* ScriptName = "AICombat::BigBrawlerStateMachine";

        Canis::Entity* hammerVisual = nullptr;

        explicit BigBrawlerStateMachine(Canis::Entity& _entity);

        BigIdleState idleState;
        BigChaseState chaseState;
        BigHammerTimeState hammerTimeState;

        void Ready() override;
        void Destroy() override;

        float GetAttackRange() const;

        void ResetHammerPose();
        void SetHammerSwing(float _normalized);
    };

    void RegisterBigBrawlerStateMachineScript(Canis::App& _app);
    void UnRegisterBigBrawlerStateMachineScript(Canis::App& _app);
}