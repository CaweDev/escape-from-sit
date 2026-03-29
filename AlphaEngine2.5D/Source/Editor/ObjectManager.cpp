#include "ObjectManager.hpp"
#include "GameEditor.hpp"
#include "imgui.h"
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

#include "Reflection.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Component.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Graphics/SpriteSheetRenderer.hpp"
#include "Engine/Input/KeyCode.hpp"

namespace AEEditor
{
    Engine::GameObject *ObjectManager::selectedObject = nullptr;
    Engine::GameObject *ObjectManager::draggingObject = nullptr;
    std::vector<Engine::GameObject *> ObjectManager::loadedObjects;

    Json::Value ObjectManager::clipboardObject;
    bool ObjectManager::hasClipboard = false;

    // =========================================================================
    // CORE SERIALIZATION HELPERS
    // =========================================================================

    Json::Value ObjectManager::SerializeGameObject(Engine::GameObject *go)
    {
        Json::Value goJson;
        goJson["Name"] = go->name;
        goJson["Tag"] = go->tag;
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
                case DataType::Color:
                {
                    auto *c = static_cast<Engine::Color *>(addr);
                    compJson[field.name]["r"] = c->r;
                    compJson[field.name]["g"] = c->g;
                    compJson[field.name]["b"] = c->b;
                    compJson[field.name]["a"] = c->a;
                    break;
                }
                case DataType::Sprite:
                case DataType::SpriteSheet:
                case DataType::String:
                    compJson[field.name] = *static_cast<std::string *>(addr);
                    break;
                }
            }
            compArray.append(compJson);
        }
        return goJson;
    }

    Engine::GameObject *ObjectManager::DeserializeGameObject(const Json::Value &goJson)
    {
        auto *go = Engine::SceneManager::GetActiveScene()->CreateGameObject(goJson["Name"].asString());
        go->active = goJson["Active"].asBool();
        go->tag = goJson.get("Tag", "Untagged").asString();

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
                case DataType::Color:
                {
                    auto *c = static_cast<Engine::Color *>(addr);
                    c->r = val["r"].asFloat();
                    c->g = val["g"].asFloat();
                    c->b = val["b"].asFloat();
                    c->a = val["a"].asFloat();
                    break;
                }
                case DataType::Sprite:
                {
                    std::string path = val.asString();
                    *static_cast<std::string *>(addr) = path;
                    if (auto *sr = dynamic_cast<Engine::SpriteRenderer *>(comp))
                    {
                        sr->SetSprite(Engine::AssetManager::GetSprite(path));
                    }
                    break;
                }
                case DataType::SpriteSheet:
                {
                    std::string path = val.asString();
                    *static_cast<std::string *>(addr) = path;
                    if (auto *ssr = dynamic_cast<Engine::SpriteSheetRenderer *>(comp))
                    {
                        ssr->SetSpriteSheet(Engine::AssetManager::GetSpriteSheet(path));
                    }
                    break;
                }
                case DataType::String:
                {
                    *static_cast<std::string *>(addr) = val.asString();
                    break;
                }
                case DataType::KeyCode:
                {
                    *static_cast<Engine::KeyCode *>(addr) = static_cast<Engine::KeyCode>(val.asInt());
                    break;
                }
                }
            }
        }
        return go;
    }

    // =========================================================================
    // EDITOR UI
    // =========================================================================

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
            loadedObjects.push_back(newObj);
            selectedObject = newObj;
        }
        ImGui::Separator();

        ImGuiIO &io = ImGui::GetIO();

        // Keyboard Shortcuts (Delete, Copy, Paste)
        if (!io.WantTextInput)
        {
            if (selectedObject && ImGui::IsKeyPressed(ImGuiKey_Delete))
            {
                ImGui::OpenPopup("Delete Confirmation");
            }

            // COPY
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
            {
                if (selectedObject && !selectedObject->isPendingDestruction)
                {
                    clipboardObject = SerializeGameObject(selectedObject);
                    hasClipboard = true;
                }
            }

            // PASTE
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V))
            {
                if (hasClipboard)
                {
                    Engine::GameObject *newGo = DeserializeGameObject(clipboardObject);
                    newGo->name += " (Copy)"; // Distinguish the copy

                    loadedObjects.push_back(newGo);
                    selectedObject = newGo;
                    SaveSceneToJson("Assets/Config/Scenes/EditorScene.json");
                }
            }
        }

        if (ImGui::BeginPopupModal("Delete Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Are you sure you want to delete '%s'?", selectedObject->name.c_str());
            ImGui::Separator();

            if (ImGui::Button("Yes", ImVec2(120, 0)))
            {
                selectedObject->isPendingDestruction = true;
                loadedObjects.erase(std::remove(loadedObjects.begin(), loadedObjects.end(), selectedObject), loadedObjects.end());
                selectedObject = nullptr;
                GameEditor::currentTileValue = -1;

                SaveSceneToJson("Assets/Config/Scenes/EditorScene.json");
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        for (auto *obj : loadedObjects)
        {
            if (!obj || obj->isPendingDestruction)
                continue;

            bool isSelected = (selectedObject == obj);

            if (ImGui::Selectable(obj->name.c_str(), isSelected))
            {
                selectedObject = obj;
                GameEditor::currentTileValue = -1;
            }

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

        DrawGameObjectHeader();

        Engine::Component *componentToRemove = nullptr;

        for (const auto &compPtr : selectedObject->GetComponents())
        {
            Engine::Component *comp = compPtr.get();
            if (!comp)
                continue;

            auto it = Registry::types.find(std::type_index(typeid(*comp)));
            if (it == Registry::types.end())
                continue;

            DrawComponent(comp, it->second, componentToRemove);
        }

        if (componentToRemove)
        {
            auto it = Registry::types.find(std::type_index(typeid(*componentToRemove)));
            if (it != Registry::types.end())
            {
                it->second.remove(selectedObject);
                SaveSceneToJson("Assets/Config/Scenes/EditorScene.json");
            }
        }

        ImGui::End();
    }

    void ObjectManager::DrawGameObjectHeader()
    {
        ImGui::Checkbox("Active", &selectedObject->active);

        char nameBuf[256];
        strncpy_s(nameBuf, selectedObject->name.c_str(), sizeof(nameBuf));
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        ImGui::Text("Name");
        if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf)))
            selectedObject->name = nameBuf;

        char tagBuf[256];
        strncpy_s(tagBuf, selectedObject->tag.c_str(), sizeof(tagBuf));
        tagBuf[sizeof(tagBuf) - 1] = '\0';
        ImGui::Text("Tag");
        if (ImGui::InputText("##Tag", tagBuf, sizeof(tagBuf)))
            selectedObject->tag = tagBuf;

        ImGui::Separator();

        if (ImGui::Button("Add Component", ImVec2(-1, 30)))
            ImGui::OpenPopup("AddComponentMenu");

        if (ImGui::BeginPopup("AddComponentMenu"))
        {
            for (const auto &[name, typeIdx] : Registry::nameToType)
            {
                if (name == "Transform")
                    continue;

                if (ImGui::Selectable(name.c_str()))
                {
                    Registry::types[typeIdx].instantiate(selectedObject);
                }
            }
            ImGui::EndPopup();
        }
    }

    void ObjectManager::DrawComponent(Engine::Component *comp, const TypeDescriptor &meta, Engine::Component *&componentToRemove)
    {
        ImGui::PushID(comp);

        bool isHeaderOpen = ImGui::CollapsingHeader(meta.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

        if (ImGui::BeginPopupContextItem("ComponentContextMenu"))
        {
            if (meta.name == "Transform")
            {
                ImGui::TextDisabled("Cannot remove Transform");
            }
            else if (ImGui::MenuItem("Remove Component"))
            {
                componentToRemove = comp;
            }
            ImGui::EndPopup();
        }

        if (isHeaderOpen)
        {
            ImGui::Checkbox("Enabled", &comp->enabled);
            for (const auto &field : meta.fields)
            {
                DrawField(comp, field);
            }
        }
        ImGui::PopID();
    }

    void ObjectManager::DrawField(Engine::Component *comp, const FieldDescriptor &field)
    {
        void *rawAddress = reinterpret_cast<void *>(reinterpret_cast<char *>(comp) + field.offset);
        ImGui::Text(field.name.c_str());
        ImGui::PushID(field.name.c_str());
        ImGui::SetNextItemWidth(-FLT_MIN);

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
        case DataType::Color:
        {
            auto *val = static_cast<Engine::Color *>(rawAddress);
            float col[4] = {val->r, val->g, val->b, val->a};
            if (ImGui::ColorEdit4("##", col))
            {
                val->r = col[0];
                val->g = col[1];
                val->b = col[2];
                val->a = col[3];
            }
            break;
        }
        case DataType::Sprite:
        {
            std::string *path = static_cast<std::string *>(rawAddress);
            std::string displayLabel = path->empty() ? "Select Sprite..." : *path;

            if (ImGui::BeginCombo("##SpriteCombo", displayLabel.c_str()))
            {
                if (std::filesystem::exists("Assets"))
                {
                    for (const auto &entry : std::filesystem::recursive_directory_iterator("Assets"))
                    {
                        if (!entry.is_regular_file())
                            continue;

                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

                        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                        {
                            std::string filePath = entry.path().string();
                            std::replace(filePath.begin(), filePath.end(), '\\', '/');

                            bool isSelected = (*path == filePath);
                            if (ImGui::Selectable(filePath.c_str(), isSelected))
                            {
                                *path = filePath;
                                if (auto *sr = dynamic_cast<Engine::SpriteRenderer *>(comp))
                                {
                                    sr->SetSprite(Engine::AssetManager::GetSprite(*path));
                                }
                            }
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DND_ASSET_SPRITE"))
                {
                    const char *droppedPath = (const char *)payload->Data;
                    *path = droppedPath;
                    if (auto *sr = dynamic_cast<Engine::SpriteRenderer *>(comp))
                    {
                        sr->SetSprite(Engine::AssetManager::GetSprite(*path));
                    }
                }
                ImGui::EndDragDropTarget();
            }
            break;
        }

        case DataType::SpriteSheet:
        {
            std::string *path = static_cast<std::string *>(rawAddress);
            std::string displayLabel = path->empty() ? "Select SpriteSheet..." : *path;

            if (ImGui::BeginCombo("##SpriteSheetCombo", displayLabel.c_str()))
            {
                if (std::filesystem::exists("Assets"))
                {
                    for (const auto &entry : std::filesystem::recursive_directory_iterator("Assets"))
                    {
                        if (!entry.is_regular_file())
                            continue;

                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

                        // Assuming your spritesheets are defined by images or json configs
                        if (ext == ".png" || ext == ".json")
                        {
                            std::string filePath = entry.path().string();
                            std::replace(filePath.begin(), filePath.end(), '\\', '/');

                            bool isSelected = (*path == filePath);
                            if (ImGui::Selectable(filePath.c_str(), isSelected))
                            {
                                *path = filePath;
                                if (auto *ssr = dynamic_cast<Engine::SpriteSheetRenderer *>(comp))
                                {
                                    ssr->SetSpriteSheet(Engine::AssetManager::GetSpriteSheet(*path));
                                }
                            }
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DND_ASSET_SPRITESHEET"))
                {
                    const char *droppedPath = (const char *)payload->Data;
                    *path = droppedPath;
                    if (auto *ssr = dynamic_cast<Engine::SpriteSheetRenderer *>(comp))
                    {
                        ssr->SetSpriteSheet(Engine::AssetManager::GetSpriteSheet(*path));
                    }
                }
                ImGui::EndDragDropTarget();
            }
            break;
        }
        case DataType::String:
        {
            std::string *str = static_cast<std::string *>(rawAddress);
            char buf[256];
            strncpy_s(buf, str->c_str(), sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';

            if (ImGui::InputText("##", buf, sizeof(buf)))
            {
                *str = buf;
            }
            break;
        }
        case DataType::KeyCode:
        {
            int *val = reinterpret_cast<int *>(rawAddress);
            ImGui::DragInt("##", val, 1.0f, 0, 255, "Key Code: %d");
            break;
        }
        }
        ImGui::PopID();
    }

    // =========================================================================
    // FILE I/O
    // =========================================================================

    void ObjectManager::SaveSceneToJson(const char *filepath)
    {
        Json::Value root;
        Json::Value &goArray = root["GameObjects"];

        for (auto *go : loadedObjects)
        {
            if (!go || go->isPendingDestruction)
                continue;

            goArray.append(SerializeGameObject(go));
        }

        std::filesystem::path pathObj(filepath);
        if (pathObj.has_parent_path())
        {
            std::filesystem::create_directories(pathObj.parent_path());
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

        loadedObjects.clear();
        selectedObject = nullptr;

        const Json::Value &goArray = root["GameObjects"];
        for (const auto &goJson : goArray)
        {
            loadedObjects.push_back(DeserializeGameObject(goJson));
        }
    }
}