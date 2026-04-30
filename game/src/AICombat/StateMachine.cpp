#include <AICombat/StateMachine.hpp>

#include <Canis/AudioManager.hpp>
#include <Canis/Debug.hpp>

#include <algorithm>
#include <cmath>

namespace AICombat
{
    StateMachine::StateMachine(Canis::Entity& _entity) :
        SuperPupUtilities::StateMachine(_entity) {}

    void StateMachine::Create()
    {
        entity.GetComponent<Canis::Transform>();

        Canis::Rigidbody& rigidbody = entity.GetComponent<Canis::Rigidbody>();
        rigidbody.motionType = Canis::RigidbodyMotionType::KINEMATIC;
        rigidbody.useGravity = false;
        rigidbody.allowSleeping = false;
        rigidbody.linearVelocity = Canis::Vector3(0.0f);
        rigidbody.angularVelocity = Canis::Vector3(0.0f);

        entity.GetComponent<Canis::BoxCollider>().size = bodyColliderSize;

        if (entity.HasComponent<Canis::Material>())
        {
            m_baseColor = entity.GetComponent<Canis::Material>().color;
            m_hasBaseColor = true;
        }
    }

    void StateMachine::Ready()
    {
        if (entity.HasComponent<Canis::Material>())
        {
            m_baseColor = entity.GetComponent<Canis::Material>().color;
            m_hasBaseColor = true;
        }

        m_currentHealth = std::max(maxHealth, 1);
        m_stateTime = 0.0f;
        m_useFirstHitSfx = true;
    }

    void StateMachine::Destroy()
    {
        SuperPupUtilities::StateMachine::Destroy();
    }

    void StateMachine::Update(float _dt)
    {
        if (!IsAlive())
            return;

        m_stateTime += _dt;
        SuperPupUtilities::StateMachine::Update(_dt);
    }

    Canis::Entity* StateMachine::FindClosestTarget() const
    {
        if (targetTag.empty() || !entity.HasComponent<Canis::Transform>())
            return nullptr;

        const Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 origin = transform.GetGlobalPosition();

        Canis::Entity* closestTarget = nullptr;
        float closestDistance = detectionRange;

        for (Canis::Entity* candidate : entity.scene.GetEntitiesWithTag(targetTag))
        {
            if (candidate == nullptr || candidate == &entity || !candidate->active)
                continue;

            if (!candidate->HasComponent<Canis::Transform>())
                continue;

            const StateMachine* other = candidate->GetScript<StateMachine>();
            if (other != nullptr && !other->IsAlive())
                continue;

            const Canis::Vector3 candidatePosition =
                candidate->GetComponent<Canis::Transform>().GetGlobalPosition();

            const float distance = glm::distance(origin, candidatePosition);

            if (distance > detectionRange || distance >= closestDistance)
                continue;

            closestDistance = distance;
            closestTarget = candidate;
        }

        return closestTarget;
    }

    float StateMachine::DistanceTo(const Canis::Entity& _other) const
    {
        if (!entity.HasComponent<Canis::Transform>() ||
            !_other.HasComponent<Canis::Transform>())
            return std::numeric_limits<float>::max();

        const Canis::Vector3 selfPosition =
            entity.GetComponent<Canis::Transform>().GetGlobalPosition();

        const Canis::Vector3 targetPosition =
            _other.GetComponent<Canis::Transform>().GetGlobalPosition();

        return glm::distance(selfPosition, targetPosition);
    }

    void StateMachine::FaceTarget(const Canis::Entity& _target)
    {
        if (!entity.HasComponent<Canis::Transform>() ||
            !_target.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();

        const Canis::Vector3 selfPosition = transform.GetGlobalPosition();

        Canis::Vector3 direction =
            _target.GetComponent<Canis::Transform>().GetGlobalPosition() - selfPosition;

        direction.y = 0.0f;

        if (glm::dot(direction, direction) <= 0.0001f)
            return;

        direction = glm::normalize(direction);

        transform.rotation.y = std::atan2(-direction.x, -direction.z);
    }

    void StateMachine::MoveTowards(const Canis::Entity& _target, float _speed, float _dt)
    {
        if (!entity.HasComponent<Canis::Transform>() ||
            !_target.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();

        const Canis::Vector3 selfPosition = transform.GetGlobalPosition();

        Canis::Vector3 direction =
            _target.GetComponent<Canis::Transform>().GetGlobalPosition() - selfPosition;

        direction.y = 0.0f;

        if (glm::dot(direction, direction) <= 0.0001f)
            return;

        direction = glm::normalize(direction);

        transform.position += direction * _speed * _dt;
    }

    void StateMachine::ChangeState(const std::string& _stateName)
    {
        if (SuperPupUtilities::StateMachine::GetCurrentStateName() == _stateName)
            return;

        if (!SuperPupUtilities::StateMachine::ChangeState(_stateName))
            return;

        m_stateTime = 0.0f;

        if (logStateChanges)
            Canis::Debug::Log("%s -> %s",
                entity.name.c_str(),
                _stateName.c_str());
    }

    const std::string& StateMachine::GetCurrentStateName() const
    {
        return SuperPupUtilities::StateMachine::GetCurrentStateName();
    }

    float StateMachine::GetStateTime() const
    {
        return m_stateTime;
    }

    int StateMachine::GetCurrentHealth() const
    {
        return m_currentHealth;
    }

    int StateMachine::GetMaxHealth() const
    {
        return maxHealth;
    }

    void StateMachine::TakeDamage(int _damage)
    {
        if (!IsAlive())
            return;

        const int damageToApply = std::max(_damage, 0);

        if (damageToApply <= 0)
            return;

        m_currentHealth = std::max(0, m_currentHealth - damageToApply);

        PlayHitSfx();

        if (m_hasBaseColor && entity.HasComponent<Canis::Material>())
        {
            Canis::Material& material = entity.GetComponent<Canis::Material>();

            const float healthRatio =
                (maxHealth > 0)
                ? static_cast<float>(m_currentHealth) / static_cast<float>(maxHealth)
                : 0.0f;

            material.color = Canis::Vector4(
                m_baseColor.x * (0.5f + (0.5f * healthRatio)),
                m_baseColor.y * (0.5f + (0.5f * healthRatio)),
                m_baseColor.z * (0.5f + (0.5f * healthRatio)),
                m_baseColor.w);
        }

        if (m_currentHealth > 0)
            return;

        if (logStateChanges)
            Canis::Debug::Log("%s was defeated.", entity.name.c_str());

        SpawnDeathEffect();
        entity.Destroy();
    }

    void StateMachine::Heal(int _amount)
    {
        if (!IsAlive())
            return;

        const int healAmount = std::max(_amount, 0);

        if (healAmount <= 0)
            return;

        m_currentHealth = std::min(maxHealth, m_currentHealth + healAmount);

        if (m_hasBaseColor && entity.HasComponent<Canis::Material>())
        {
            Canis::Material& material = entity.GetComponent<Canis::Material>();

            const float healthRatio =
                (maxHealth > 0)
                ? static_cast<float>(m_currentHealth) / static_cast<float>(maxHealth)
                : 0.0f;

            material.color = Canis::Vector4(
                m_baseColor.x * (0.5f + (0.5f * healthRatio)),
                m_baseColor.y * (0.5f + (0.5f * healthRatio)),
                m_baseColor.z * (0.5f + (0.5f * healthRatio)),
                m_baseColor.w);
        }
    }

    void StateMachine::PlayHitSfx()
    {
        const Canis::AudioAssetHandle& selectedSfx =
            m_useFirstHitSfx ? hitSfxPath1 : hitSfxPath2;

        m_useFirstHitSfx = !m_useFirstHitSfx;

        if (selectedSfx.Empty())
            return;

        Canis::AudioManager::PlaySFX(
            selectedSfx,
            std::clamp(hitSfxVolume, 0.0f, 1.0f));
    }

    void StateMachine::SpawnDeathEffect()
    {
        if (deathEffectPrefab.Empty() || !entity.HasComponent<Canis::Transform>())
            return;

        const Canis::Transform& sourceTransform =
            entity.GetComponent<Canis::Transform>();

        const Canis::Vector3 spawnPosition =
            sourceTransform.GetGlobalPosition();

        const Canis::Vector3 spawnRotation =
            sourceTransform.GetGlobalRotation();

        for (Canis::Entity* spawnedEntity :
            entity.scene.Instantiate(deathEffectPrefab))
        {
            if (spawnedEntity == nullptr ||
                !spawnedEntity->HasComponent<Canis::Transform>())
                continue;

            Canis::Transform& spawnedTransform =
                spawnedEntity->GetComponent<Canis::Transform>();

            spawnedTransform.position = spawnPosition;
            spawnedTransform.rotation = spawnRotation;
        }
    }

    bool StateMachine::IsAlive() const
    {
        return m_currentHealth > 0;
    }
}