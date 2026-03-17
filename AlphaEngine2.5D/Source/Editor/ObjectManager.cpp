#include "ObjectManager.hpp"
#include "GameEditor.hpp"
#include "imgui.h"
#include <cstring>
#include <json/json.h>
#include <fstream>

#include "Reflection.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Component.hpp"
#include "Engine/Scene/SceneManager.hpp"
namespace AEEditor
{

    Engine::GameObject *ObjectManager::selectedObject = nullptr;
    Engine::GameObject *ObjectManager::draggingObject = nullptr;
    void ObjectManager::DrawHierarchy()
    {
        ImGui::Begin("Hierarchy");
        if (ImGui::Button("Save Scene", ImVec2(-1, 30)))
        {
            SaveSceneToJson("Assets/Config/Scenes/EditorScene.json");
        }
        ImGui::Separator();

        if (ImGui::Button("Create Empty GameObject", ImVec2(-1, 0)))
        {
            auto *newObj = Engine::SceneManager::GetActiveScene()->CreateGameObject("New GameObject");
            selectedObject = newObj;
        }
        ImGui::Separator();

        const auto &allObjects = Engine::GameObject::GetAllGameObjects();

        for (auto *obj : allObjects)
        {
            if (!obj || obj->isPendingDestruction)
                continue;

            bool isSelected = (selectedObject == obj);

            // Render selectable row
            if (ImGui::Selectable(obj->name.c_str(), isSelected))
            {
                selectedObject = obj;
                GameEditor::currentTileValue = -1;
            }

            // Keep focus if selected
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("DND_GAMEOBJECT", &obj, sizeof(Engine::GameObject *));
                ImGui::Text("Moving %s", obj->name.c_str());
                ImGui::EndDragDropSource();
            }
        }

        ImGui::End();
    }

    void ObjectManager::DrawInspector()
    {
        ImGui::Begin("Inspector");

        if (!selectedObject || selectedObject->isPendingDestruction)
        {
            ImGui::End();
            return;
        }

        ImGui::Checkbox("Active", &selectedObject->active);

        char nameBuf[256];
        strncpy_s(nameBuf, selectedObject->name.c_str(), sizeof(nameBuf));
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        ImGui::Text("Name");
        if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf)))
            selectedObject->name = nameBuf;

        ImGui::Separator();

        if (ImGui::Button("Add Component", ImVec2(-1, 30)))
            ImGui::OpenPopup("AddComponentMenu");

        if (ImGui::BeginPopup("AddComponentMenu"))
        {
            for (const auto &[name, typeIdx] : Registry::nameToType)
            {
                // Exclude Transform to prevent duplicates
                if (name == "Transform")
                    continue;

                if (ImGui::Selectable(name.c_str()))
                {
                    Registry::types[typeIdx].instantiate(selectedObject);
                }
            }
            ImGui::EndPopup();
        }

        for (const auto &compPtr : selectedObject->GetComponents())
        {
            Engine::Component *comp = compPtr.get();
            if (!comp)
                continue;

            // 1. Lookup metadata via RTTI
            auto it = Registry::types.find(std::type_index(typeid(*comp)));
            if (it == Registry::types.end())
                continue; // Unregistered component

            const TypeDescriptor &meta = it->second;

            ImGui::PushID(comp);
            if (ImGui::CollapsingHeader(meta.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {

                ImGui::Checkbox("Enabled", &comp->enabled);

                // 2. Iterate memory offsets
                for (const auto &field : meta.fields)
                {

                    // 3. Calculate absolute memory address of the specific variable
                    void *rawAddress = reinterpret_cast<void *>(reinterpret_cast<char *>(comp) + field.offset);
                    ImGui::Text(field.name.c_str());
                    ImGui::PushID(field.name.c_str());
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    // 4. Cast and draw based on enum
                    switch (field.type)
                    {
                    case DataType::Float:
                    {
                        float *val = static_cast<float *>(rawAddress);

                        ImGui::DragFloat("##", val, 0.1f);
                        break;
                    }
                    case DataType::Int:
                    {
                        int *val = static_cast<int *>(rawAddress);

                        ImGui::DragInt("##", val);
                        break;
                    }
                    case DataType::Bool:
                    {
                        bool *val = static_cast<bool *>(rawAddress);

                        ImGui::Checkbox("##", val);
                        break;
                    }
                    case DataType::Vector2:
                    {
                        auto *val = static_cast<Engine::Vector2 *>(rawAddress);
                        float vec[2] = {val->x, val->y};

                        if (ImGui::DragFloat2("##", vec, 0.1f))
                        {
                            val->x = vec[0];
                            val->y = vec[1];
                        }
                        break;
                    }
                    case DataType::Vector3:
                    {
                        auto *val = static_cast<Engine::Vector3 *>(rawAddress);
                        float vec[3] = {val->x, val->y, val->z};

                        if (ImGui::DragFloat3("##", vec, 0.1f))
                        {
                            val->x = vec[0];
                            val->y = vec[1];
                            val->z = vec[2];
                        }
                        break;
                    }
                    }

                    ImGui::PopID();
                }
            }
            ImGui::PopID();
        }

        ImGui::End();
    }

    void ObjectManager::SaveSceneToJson(const char *filepath)
    {
        Json::Value root;
        Json::Value &goArray = root["GameObjects"];

        for (auto *go : Engine::GameObject::GetAllGameObjects())
        {
            if (!go || go->isPendingDestruction)
                continue;

            Json::Value goJson;
            goJson["Name"] = go->name;
            goJson["Active"] = go->active;

            Json::Value &compArray = goJson["Components"];
            for (const auto &compPtr : go->GetComponents())
            {
                Engine::Component *comp = compPtr.get();
                if (!comp)
                    continue;

                auto it = Registry::types.find(std::type_index(typeid(*comp)));
                if (it == Registry::types.end())
                    continue;

                Json::Value compJson;
                compJson["Type"] = it->second.name;

                for (const auto &field : it->second.fields)
                {
                    void *addr = reinterpret_cast<void *>(reinterpret_cast<char *>(comp) + field.offset);
                    switch (field.type)
                    {
                    case DataType::Int:
                        compJson[field.name] = *static_cast<int *>(addr);
                        break;
                    case DataType::Float:
                        compJson[field.name] = *static_cast<float *>(addr);
                        break;
                    case DataType::Bool:
                        compJson[field.name] = *static_cast<bool *>(addr);
                        break;
                    case DataType::Vector2:
                    {
                        auto *v = static_cast<Engine::Vector2 *>(addr);
                        compJson[field.name]["x"] = v->x;
                        compJson[field.name]["y"] = v->y;
                        break;
                    }
                    case DataType::Vector3:
                    {
                        auto *v = static_cast<Engine::Vector3 *>(addr);
                        compJson[field.name]["x"] = v->x;
                        compJson[field.name]["y"] = v->y;
                        compJson[field.name]["z"] = v->z;
                        break;
                    }
                    }
                }
                compArray.append(compJson);
            }
            goArray.append(goJson);
        }

        std::ofstream file(filepath, std::ios::out | std::ios::trunc);
        if (file.is_open())
        {
            file << root;
            file.close();
        }
    }

    void ObjectManager::LoadSceneFromJson(const char *filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
            return;

        Json::Value root;
        file >> root;
        file.close();

        const Json::Value &goArray = root["GameObjects"];
        for (const auto &goJson : goArray)
        {
            auto *go = Engine::SceneManager::GetActiveScene()->CreateGameObject(goJson["Name"].asString());
            go->active = goJson["Active"].asBool();

            const Json::Value &compArray = goJson["Components"];
            for (const auto &compJson : compArray)
            {
                std::string typeName = compJson["Type"].asString();

                auto it = Registry::nameToType.find(typeName);
                if (it == Registry::nameToType.end())
                    continue;

                const TypeDescriptor &meta = Registry::types[it->second];

                Engine::Component *comp = nullptr;
                if (typeName == "Transform")
                {
                    comp = go->transform;
                }
                else
                {
                    comp = meta.instantiate(go);
                }

                for (const auto &field : meta.fields)
                {
                    if (!compJson.isMember(field.name))
                        continue;

                    void *addr = reinterpret_cast<void *>(reinterpret_cast<char *>(comp) + field.offset);
                    const Json::Value &val = compJson[field.name];

                    switch (field.type)
                    {
                    case DataType::Int:
                        *static_cast<int *>(addr) = val.asInt();
                        break;
                    case DataType::Float:
                        *static_cast<float *>(addr) = val.asFloat();
                        break;
                    case DataType::Bool:
                        *static_cast<bool *>(addr) = val.asBool();
                        break;
                    case DataType::Vector2:
                    {
                        auto *v = static_cast<Engine::Vector2 *>(addr);
                        v->x = val["x"].asFloat();
                        v->y = val["y"].asFloat();
                        break;
                    }
                    case DataType::Vector3:
                    {
                        auto *v = static_cast<Engine::Vector3 *>(addr);
                        v->x = val["x"].asFloat();
                        v->y = val["y"].asFloat();
                        v->z = val["z"].asFloat();
                        break;
                    }
                    }
                }
            }
        }
    }
}