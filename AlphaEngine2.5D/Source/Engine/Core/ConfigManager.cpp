/**
 * @file ConfigManager.cpp
 * @brief Implementation of ConfigManager file parsing using jsoncpp.
 */

#include "ConfigManager.hpp"
#include <fstream>

namespace Engine
{
    bool ConfigManager::LoadJson(const std::string& filepath, Json::Value& root)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            printf("[Warning] ConfigManager: Could not open file: %s\n", filepath.c_str());
            return false;
        }

        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        if (!parseFromStream(builder, file, &root, &errs))
        {
            printf("[Warning] ConfigManager: Failed to parse JSON file for %s: %s\n", filepath.c_str(), errs.c_str());
            return false;
        }
        
        return true;
    }
} // namespace Engine
