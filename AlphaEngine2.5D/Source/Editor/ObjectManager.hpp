#pragma once
#include <vector>
#include "Reflection.hpp"
namespace Engine
{
    class GameObject;
    class Component;
}

namespace AEEditor
{
    class ObjectManager
    {
    public:
        static Engine::GameObject *selectedObject;
        static Engine::GameObject *draggingObject;
        static std::vector<Engine::GameObject *> loadedObjects;
        static void DrawHierarchy();
        static void DrawInspector();
        static void SaveSceneToJson(const char *filepath);
        static void LoadSceneFromJson(const char *filepath);

    private:
        static void DrawGameObjectHeader();
        static void DrawComponent(Engine::Component *comp, const TypeDescriptor &meta, Engine::Component *&componentToRemove);
        static void DrawField(Engine::Component *comp, const FieldDescriptor &field);
    };

}