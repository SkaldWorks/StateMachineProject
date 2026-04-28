#pragma once

#include <AICombat/BrawlerStateMachine.hpp>

namespace AICombat
{
    class BigBrawlerStateMachine : public BrawlerStateMachine
    {
    public:
        static constexpr const char* ScriptName = "AICombat::BigBrawlerStateMachine";

        explicit BigBrawlerStateMachine(Canis::Entity& _entity)
            : BrawlerStateMachine(_entity)
        {
            maxHealth = 80;
            chaseState.moveSpeed = 2.0f;
        }
    };

    void RegisterBigBrawlerStateMachineScript(Canis::App& _app);
    void UnRegisterBigBrawlerStateMachineScript(Canis::App& _app);
}