#ifndef GRID_H
#define GRID_H

#include <vector>

#include "Primitive.h"
#include "Camera.h"

class Grid
{
public:
    Grid( std::vector< Primitive* > primitives, glm::vec3 b0, glm::vec3 b1 );
    ~Grid();

    int GetGridArrayLength() const { return m_arrayLength; }
    int* GetGridArray() const { return m_grid; }
    std::vector< int > GetObjectRefVector() const { return m_cellObjectRefs; }

    void Update( std::vector< Primitive* > primitives, const glm::vec3& camPos, const glm::vec3& camDir );
    void Draw();

private:
    void buildGrid( std::vector< Primitive* > primitives );
    void traverseGrid();
    void drawCube( const glm::vec3& p0, const glm::vec3& p1 );

    int m_arrayLength;
    int* m_grid;
    glm::vec3 m_p0;
    glm::vec3 m_p1;
    glm::vec3 m_cellSize;
    glm::vec3 m_cellDelta;
    std::vector< int > m_cellObjectRefs;
};

#endif // GRID_H
