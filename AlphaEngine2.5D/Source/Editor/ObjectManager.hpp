#pragma once
#include "Engine/Core/GameObject.hpp"



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
    };
}