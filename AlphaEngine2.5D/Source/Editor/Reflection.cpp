#include "Reflection.hpp"

namespace AEEditor
{
    std::unordered_map<std::type_index, TypeDescriptor> Registry::types;
    std::unordered_map<std::string, std::type_index> Registry::nameToType;
}