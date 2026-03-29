/* Start Header ************************************************************************/
/*!
\file		Reflection.cpp
\author 	
\par	
\date	March, 29, 2026
\brief		[AUTO] Reflection

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "Reflection.hpp"

namespace AEEditor
{
    std::unordered_map<std::type_index, TypeDescriptor> Registry::types;
    std::unordered_map<std::string, std::type_index> Registry::nameToType;
}
