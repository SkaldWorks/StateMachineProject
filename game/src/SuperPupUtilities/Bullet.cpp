#include <SuperPupUtilities/Bullet.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>

#include <algorithm>
#include <string>

#include <AICombat/StateMachine.hpp>

namespace SuperPupUtilities
{
    namespace
    {
        YAML::Node EncodeTargetTags(const std::vector<std::string>& _tags)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (const std::string& tag : _tags)
                node.push_back(tag);
            return node;
        }

        std::vector<std::string> DecodeTargetTags(const YAML::Node& _node)
        {
            std::vector<std::string> tags = {};
            if (!_node || !_node.IsSequence())
                return tags;

            for (const YAML::Node& tagNode : _node)
                tags.push_back(tagNode.as<std::string>(""));

            return tags;
        }
    }

    Canis::ScriptConf bulletConf = {};

    void RegisterBulletScript(Canis::App& _app)
    {
        DEFAULT_CONFIG_AND_REQUIRED(bulletConf, SuperPupUtilities::Bullet, Canis::Transform);

        bulletConf.Encode = [](YAML::Node& _node, Canis::Entity& _entity) -> void
        {
            Bullet* component = _entity.GetScript<Bullet>();
            if (component == nullptr)
                return;

            YAML::Node bulletNode;
            bulletNode["damage"] = component->damage;
            bulletNode["speed"] = component->speed;
            bulletNode["lifeTime"] = component->lifeTime;
            bulletNode["gravity"] = component->gravity;
            bulletNode["destroyOnImpact"] = component->destroyOnImpact;
            bulletNode["destroyEntityWhenDone"] = component->destroyEntityWhenDone;
            bulletNode["autoLaunch"] = component->autoLaunch;
            bulletNode["collisionMask"] = component->collisionMask;
            bulletNode["hitImpulse"] = component->hitImpulse;
            bulletNode["targetTags"] = EncodeTargetTags(component->targetTags);
            _node[Bullet::ScriptName] = bulletNode;
        };

        bulletConf.Decode = [](YAML::Node& _node, Canis::Entity& _entity, bool _callCreate) -> void
        {
            YAML::Node bulletNode = _node[Bullet::ScriptName];
            if (!bulletNode)
                return;

            Bullet* component = _entity.GetScript<Bullet>();
            if (component == nullptr)
                component = _entity.AddScript<Bullet>(_callCreate);

            if (component == nullptr)
                return;

            component->damage = bulletNode["damage"].as<int>(component->damage);
            component->speed = bulletNode["speed"].as<float>(component->speed);
            component->lifeTime = bulletNode["lifeTime"].as<float>(component->lifeTime);
            component->gravity = bulletNode["gravity"].as<float>(component->gravity);
            component->destroyOnImpact = bulletNode["destroyOnImpact"].as<bool>(component->destroyOnImpact);
            component->destroyEntityWhenDone = bulletNode["destroyEntityWhenDone"].as<bool>(component->destroyEntityWhenDone);
            component->autoLaunch = bulletNode["autoLaunch"].as<bool>(component->autoLaunch);
            component->collisionMask = bulletNode["collisionMask"].as<Canis::Mask>(component->collisionMask);
            component->hitImpulse = bulletNode["hitImpulse"].as<float>(component->hitImpulse);
            component->targetTags = DecodeTargetTags(bulletNode["targetTags"]);
        };

        bulletConf.DrawInspector = [](Canis::Editor& _editor, Canis::Entity& _entity, const Canis::ScriptConf& _conf) -> void
        {
            Bullet* component = _entity.GetScript<Bullet>();
            if (component == nullptr)
                return;

            DrawInspectorField(_editor, "damage", _conf.name.c_str(), component->damage);
            DrawInspectorField(_editor, "speed", _conf.name.c_str(), component->speed);
            DrawInspectorField(_editor, "lifeTime", _conf.name.c_str(), component->lifeTime);
            DrawInspectorField(_editor, "gravity", _conf.name.c_str(), component->gravity);
            DrawInspectorField(_editor, "destroyOnImpact", _conf.name.c_str(), component->destroyOnImpact);
            DrawInspectorField(_editor, "destroyEntityWhenDone", _conf.name.c_str(), component->destroyEntityWhenDone);
            DrawInspectorField(_editor, "autoLaunch", _conf.name.c_str(), component->autoLaunch);
            DrawInspectorField(_editor, "collisionMask", _conf.name.c_str(), component->collisionMask);
            DrawInspectorField(_editor, "hitImpulse", _conf.name.c_str(), component->hitImpulse);

            if (ImGui::Button(("Add Target Tag##" + _conf.name).c_str()))
                component->targetTags.emplace_back();

            for (size_t i = 0; i < component->targetTags.size(); ++i)
            {
                std::string& tag = component->targetTags[i];
                const std::string indexId = _conf.name + std::to_string(i);
                DrawInspectorField(_editor, "targetTag", indexId.c_str(), tag);

                if (ImGui::Button(("Remove##tag" + indexId).c_str()))
                {
                    component->targetTags.erase(component->targetTags.begin() + i);
                    --i;
                }
            }

            ImGui::Text("Launched: %s", component->IsLaunched() ? "Yes" : "No");
        };

        _app.RegisterScript(bulletConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(bulletConf, Bullet)

    void Bullet::Create()
    {
        entity.GetComponent<Canis::Transform>();
    }

    void Bullet::Ready()
    {
        ResetLifetime();

        if (autoLaunch && entity.active)
            Launch();
    }

    void Bullet::Destroy() {}

    void Bullet::Update(float _dt)
    {
        if (!m_launched || !entity.active || !entity.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 start = m_lastPosition;

        Move(_dt);

        const Canis::Vector3 end = transform.GetGlobalPosition();
        CollisionCheck(start, end);

        if (!entity.active)
            return;

        m_lastPosition = end;
        m_timeRemaining -= _dt;

        if (m_timeRemaining <= 0.0f)
            DestroyBullet();
    }

    void Bullet::Launch()
    {
        if (!entity.HasComponent<Canis::Transform>())
            return;

        ResetLifetime();
        m_launched = true;
        m_lastPosition = entity.GetComponent<Canis::Transform>().GetGlobalPosition();
        entity.active = true;
    }

    void Bullet::Launch(const Canis::Vector3& _position, const Canis::Vector3& _rotation)
    {
        if (!entity.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        transform.position = _position;
        transform.rotation = _rotation;
        Launch();
    }

    void Bullet::DestroyBullet()
    {
        m_launched = false;
        ResetLifetime();

        if (destroyEntityWhenDone)
        {
            entity.Destroy();
            return;
        }

        entity.active = false;
    }

    bool Bullet::IsLaunched() const
    {
        return m_launched;
    }

    void Bullet::ResetLifetime()
    {
        m_timeRemaining = lifeTime;
    }

    void Bullet::Move(float _dt)
    {
        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        transform.position += transform.GetForward() * speed * _dt;
        transform.position += Canis::Vector3(0.0f, gravity * _dt, 0.0f);
    }

    void Bullet::CollisionCheck(const Canis::Vector3& _start, const Canis::Vector3& _end)
    {
        const Canis::Vector3 travel = _end - _start;
        const float distance = glm::length(travel);
        if (distance <= 0.0001f)
            return;

        const Canis::Vector3 direction = travel / distance;
        Canis::RaycastHit hit = {};

        if (!entity.scene.Raycast(_start, direction, hit, distance, collisionMask))
            return;

        if (hit.entity == nullptr || hit.entity == &entity)
            return;

        // APPLY DAMAGE
        if (IsValidTarget(*hit.entity))
        {
            if (AICombat::StateMachine* target =
                hit.entity->GetScript<AICombat::StateMachine>())
            {
                if (target->IsAlive())
                {
                    target->TakeDamage(damage);
                }
            }

            // optional physics push
            if (hitImpulse > 0.0f && hit.entity->HasComponent<Canis::Rigidbody>())
            {
                hit.entity->GetComponent<Canis::Rigidbody>().AddForce(
                    direction * hitImpulse,
                    Canis::Rigidbody3DForceMode::IMPULSE);
            }
        }

        if (destroyOnImpact)
            DestroyBullet();
    }

    bool Bullet::IsValidTarget(const Canis::Entity& _entity) const
    {
        if (targetTags.empty())
            return true;

        return std::find(targetTags.begin(), targetTags.end(), _entity.tag) != targetTags.end();
    }
}
