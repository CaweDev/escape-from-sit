#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <cstddef>

#include "Engine/Core/Component.hpp"
#include "Engine/Core/GameObject.hpp"

namespace AEEditor
{

    enum class DataType
    {
        Int,
        Float,
        Bool,
        Vector2,
        Vector3,
        Color,
        Sprite,
        String,
        KeyCode
    };

    struct FieldDescriptor
    {
        std::string name;
        DataType type;
        size_t offset;
    };
    using InstantiateFn = Engine::Component *(*)(Engine::GameObject *);
    using RemoveFn = void (*)(Engine::GameObject *);
    struct TypeDescriptor
    {
        std::string name;
        std::vector<FieldDescriptor> fields;
        InstantiateFn instantiate;
        RemoveFn remove;
    };

    class Registry
    {
    public:
        static std::unordered_map<std::type_index, TypeDescriptor> types;
        static std::unordered_map<std::string, std::type_index> nameToType;

        template <typename ClassType>
        static void RegisterClassType(const std::string &className)
        {
            std::type_index typeIdx = std::type_index(typeid(ClassType));
            types[typeIdx]
                .name = className;

            types[typeIdx].instantiate = [](Engine::GameObject *go) -> Engine::Component *
            {
                return go->AddComponent<ClassType>();
            };
            types[typeIdx].remove = [](Engine::GameObject *go)
            {
                go->RemoveComponent<ClassType>();
            };

            nameToType.emplace(className, typeIdx);
        }

        template <typename ClassType>
        static void RegisterFieldType(const std::string &fieldName, size_t offset, DataType type)
        {
            types[std::type_index(typeid(ClassType))].fields.push_back({fieldName, type, offset});
        }
    };
}