/* Start Header ************************************************************************/
/*!
\file		ObjectManager.hpp
\author 	
\par	
\date	March, 29, 2026
\brief		[AUTO] ObjectManager

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include <vector>
#include "Reflection.hpp"
#include <json/json.h>
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
        static Json::Value clipboardObject;
        static bool hasClipboard;
        static Json::Value SerializeGameObject(Engine::GameObject *go);
        static Engine::GameObject *DeserializeGameObject(const Json::Value &goJson);
        static void DrawGameObjectHeader();
        static void DrawComponent(Engine::Component *comp, const TypeDescriptor &meta, Engine::Component *&componentToRemove);
        static void DrawField(Engine::Component *comp, const FieldDescriptor &field);
    };

}
