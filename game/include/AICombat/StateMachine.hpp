#pragma once

#include <Canis/Entity.hpp>

#include <SuperPupUtilities/StateMachine.hpp>

#include <string>
#include <limits>

namespace AICombat
{
    class StateMachine : public SuperPupUtilities::StateMachine
    {
    public:
        std::string targetTag = "";
        float detectionRange = 20.0f;
        Canis::Vector3 bodyColliderSize = Canis::Vector3(1.0f);
        int maxHealth = 20;
        bool logStateChanges = true;

        Canis::AudioAssetHandle hitSfxPath1 = { .path = "" };
        Canis::AudioAssetHandle hitSfxPath2 = { .path = "" };
        float hitSfxVolume = 1.0f;

        Canis::SceneAssetHandle deathEffectPrefab = { .path = "" };

        explicit StateMachine(Canis::Entity& _entity);

        virtual void Create() override;
        virtual void Ready() override;
        virtual void Destroy() override;
        virtual void Update(float _dt) override;

        Canis::Entity* FindClosestTarget() const;
        float DistanceTo(const Canis::Entity& _other) const;
        void FaceTarget(const Canis::Entity& _target);
        void MoveTowards(const Canis::Entity& _target, float _speed, float _dt);

        virtual void ChangeState(const std::string& _stateName);

        const std::string& GetCurrentStateName() const;
        float GetStateTime() const;
        int GetCurrentHealth() const;
        int GetMaxHealth() const;

        virtual void TakeDamage(int _damage);
        virtual void Heal(int _amount);

        bool IsAlive() const;

    protected:
        virtual void PlayHitSfx();
        virtual void SpawnDeathEffect();

        int m_currentHealth = 0;
        float m_stateTime = 0.0f;

        Canis::Vector4 m_baseColor = Canis::Vector4(1.0f);
        bool m_hasBaseColor = false;

        bool m_useFirstHitSfx = true;
    };
}