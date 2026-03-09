#pragma once
/**
 * @file ConfigManager.hpp
 * @brief Static configuration management class for loading JSON config files into C++ structs.
 * 
 * Example usage:
 * \code
 * struct MyConfig : public Engine::IConfig {
 *     int speed;
 *     std::string name;
 * 
 *     void FromJson(const Json::Value& j) override {
 *         speed = j.get("speed", 0).asInt();
 *         name = j.get("name", "Unknown").asString();
 *     }
 * };
 * 
 * // Usage:
 * MyConfig config;
 * Engine::ConfigManager::ReadConfig("config.json", config);
 * \endcode
 */

#include <string>
#include <json/json.h>

namespace Engine
{
    /**
     * @brief Abstract base struct that configuration data types should inherit from.
     * Enforces the implementation of the FromJson deserialization method.
     */
    struct IConfig
    {
        virtual ~IConfig() = default;
        virtual void FromJson(const Json::Value& json) = 0;
    };

    /**
     * @class ConfigManager
     * @brief Static class for centralized configuration loading
     */
    class ConfigManager
    {
    private:
        // Prevent instantiation
        ConfigManager() = delete;
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;

        static bool LoadJson(const std::string& filepath, Json::Value& root);

    public:
        /**
         * @brief Reads a JSON file and parses it into a struct of type T
         * @param filepath Path to the JSON file
         * @param outConfig The configuration object to populate
         * @return True if successful, false otherwise
         */
        template<typename T>
        static bool ReadConfig(const std::string& filepath, T& outConfig)
        {
            Json::Value root;
            if (!LoadJson(filepath, root))
            {
                return false;
            }

            outConfig.FromJson(root);
            return true;
        }
    };

} // namespace Engine
