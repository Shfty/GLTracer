#ifndef UTILITY_H
#define UTILITY_H

#include <glm/glm.hpp>
#include <string>

namespace Utility
{
    void PrintVector( const std::string& prefix, const glm::vec3& vec )
    {
        std::cout << prefix << vec.x << ", " << vec.y << ", " << vec.z << std::endl;
    }
}

#endif // UTILITY_H
