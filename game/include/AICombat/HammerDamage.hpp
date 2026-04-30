#pragma once

#include <Canis/Entity.hpp>

#include <vector>

namespace AICombat
{
    class StateMachine;

    class HammerDamage : public Canis::ScriptableEntity
    {
    public:
        static constexpr const char* ScriptName = "AICombat::HammerDamage";

        Canis::Entity* owner = nullptr;
        Canis::Vector3 sensorSize = Canis::Vector3(1.0f);
        int damage = 10;
        std::string targetTag = "";

        explicit HammerDamage(Canis::Entity& _entity) :
            Canis::ScriptableEntity(_entity) {}

        void Create() override;
        void Ready() override;
        void Update(float _dt) override;

        void CheckSensorEnter();

    private:
        StateMachine* GetOwnerStateMachine();
        Canis::Entity* FindOwnerFromHierarchy() const;
        bool HasDamagedThisSwing(Canis::Entity& _target) const;

        std::vector<Canis::Entity*> m_hitTargetsThisSwing = {};
    };

    void RegisterHammerDamageScript(Canis::App& _app);
    void UnRegisterHammerDamageScript(Canis::App& _app);
}