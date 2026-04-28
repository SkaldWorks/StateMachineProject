#include <AICombat/BigBrawlerStateMachine.hpp>
#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>

namespace AICombat
{
    namespace
    {
        ScriptConf bigBrawlerConf = {};
    }

    void RegisterBigBrawlerStateMachineScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(bigBrawlerConf, AICombat::BigBrawlerStateMachine, targetTag);
        REGISTER_PROPERTY(bigBrawlerConf, AICombat::BigBrawlerStateMachine, detectionRange);
        REGISTER_PROPERTY(bigBrawlerConf, AICombat::BigBrawlerStateMachine, bodyColliderSize);
        REGISTER_PROPERTY(bigBrawlerConf, AICombat::BigBrawlerStateMachine, maxHealth);

        DEFAULT_CONFIG_AND_REQUIRED(
            bigBrawlerConf,
            AICombat::BigBrawlerStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        bigBrawlerConf.DEFAULT_DRAW_INSPECTOR(AICombat::BigBrawlerStateMachine);

        _app.RegisterScript(bigBrawlerConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(bigBrawlerConf, BigBrawlerStateMachine)
}