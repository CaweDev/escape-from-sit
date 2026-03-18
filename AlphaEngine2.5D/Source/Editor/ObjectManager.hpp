#pragma once

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
        static void DrawHierarchy();
        static void DrawInspector();
        static void SaveSceneToJson(const char *filepath);
        static void LoadSceneFromJson(const char *filepath);
    };
}