#pragma once

#include <AICombat/StateMachine.hpp>

#include <vector>

namespace AICombat
{
    class HealerStateMachine;

    class HealerIdleState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "HealerIdleState";

        explicit HealerIdleState(SuperPupUtilities::StateMachine& _stateMachine);
        void Update(float _dt) override;
    };

    class HealerMoveState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "HealerMoveState";
        float moveSpeed = 3.5f;
        float followDistance = 2.0f;

        explicit HealerMoveState(SuperPupUtilities::StateMachine& _stateMachine);
        void Update(float _dt) override;
    };

    class HealerHealState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "HealerHealState";

        int healAmount = 2;
        float healInterval = 1.0f;

        explicit HealerHealState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
        void Exit() override;

    private:
        float m_timer = 0.0f;
    };

    class HealerStateMachine : public StateMachine
    {
    public:
        static constexpr const char* ScriptName = "AICombat::HealerStateMachine";

        std::string allyTag = "";
        Canis::Entity* currentHealTarget = nullptr;

        Canis::AudioAssetHandle healSfx = { .path = "" };
        float healSfxVolume = 1.0f;

        Canis::SceneAssetHandle healEffectPrefab = { .path = "" };
        float healEffectLifetime = 0.5f;

        explicit HealerStateMachine(Canis::Entity& _entity);

        HealerIdleState idleState;
        HealerMoveState moveState;
        HealerHealState healState;

        void Ready() override;
        void Destroy() override;
        void Update(float _dt) override;

        Canis::Entity* FindLowestHealthAlly() const;
        bool IsTargetBeingHealed(Canis::Entity& _target) const;
        Canis::Vector3 GetHealPosition(const Canis::Entity& _target, float _distance) const;

        void ReserveTarget(Canis::Entity* _target);
        void ClearReservedTarget();

        void SpawnHealEffect();

    private:
        struct PendingHealEffect
        {
            Canis::Entity* entity = nullptr;
            float timeRemaining = 0.0f;
        };

        std::vector<PendingHealEffect> m_pendingHealEffects = {};

        void UpdateHealEffects(float _dt);
        void DestroyAllPendingHealEffects();
    };

    void RegisterHealerStateMachineScript(Canis::App& _app);
    void UnRegisterHealerStateMachineScript(Canis::App& _app);
}