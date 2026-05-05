#pragma once

#include <AICombat/StateMachine.hpp>

namespace AICombat
{
    class MageStateMachine;

    class MageIdleState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "MageIdleState";

        explicit MageIdleState(SuperPupUtilities::StateMachine& _stateMachine);
        void Update(float _dt) override;
    };

    class MageChaseState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "MageChaseState";
        float moveSpeed = 3.75f;

        explicit MageChaseState(SuperPupUtilities::StateMachine& _stateMachine);
        void Update(float _dt) override;
    };

    class MageCastState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "MageCastState";

        float castWindupTime = 0.5f;

        explicit MageCastState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
        void Exit() override;

    private:
        bool m_hasFired = false;
    };

    class MageStateMachine : public StateMachine
    {
    public:
        static constexpr const char* ScriptName = "AICombat::MageStateMachine";

        // Combat
        std::string projectilePoolCode = "";
        float attackRange = 10.0f;

        // Spawn tuning
        float projectileSpawnDistance = 1.25f;
        float projectileSpawnHeight = 1.0f;

        // Projectile stats
        int projectileDamage = 8;
        float projectileSpeed = 18.0f;
        float projectileLifeTime = 6.0f;
        float projectileGravity = 0.0f;
        bool projectileDestroyOnImpact = true;
        bool projectileDestroyEntityWhenDone = false;
        float projectileHitImpulse = 0.0f;

        // NEW: Cast SFX
        Canis::AudioAssetHandle castSfx = { .path = "" };
        float castSfxVolume = 1.0f;

        explicit MageStateMachine(Canis::Entity& _entity);

        MageIdleState idleState;
        MageChaseState chaseState;
        MageCastState castState;

        void Ready() override;
        void Destroy() override;

        float GetAttackRange() const;
        void ShootProjectile();
    };

    void RegisterMageStateMachineScript(Canis::App& _app);
    void UnRegisterMageStateMachineScript(Canis::App& _app);
}