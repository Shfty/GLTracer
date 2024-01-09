#ifndef KDTREE_H
#define KDTREE_H

#include <glm/glm.hpp>
#include <vector>

#include "Primitive.h"

struct kdNode
{
    glm::vec4 Value;
    kdNode* LeftChild;
    kdNode* RightChild;
};

class kdTree
{
public:
    kdTree( std::vector< Primitive* > primitives );
    ~kdTree();

    std::vector< glm::vec4 > GetTreeVector() const { return m_treeVector; }
    std::vector< int > GetLeafObjectsVector() const { return m_leafObjectsVector; }

    void BuildTree( std::vector<Primitive*> primitives );
    kdNode* BuildNode( std::vector<Primitive*> primitives, int depth );

private:
    kdNode* m_rootNode;
    std::vector< glm::vec4 > m_treeVector;
    std::vector< int > m_leafObjectsVector;

    kdNode* constructBranchNode( std::vector<Primitive*> primitives, int axis, float splitPos, int depth );
    kdNode* constructLeafNode( std::vector<Primitive*> primitives );
    void constructTreeVector();
};

#endif // KDTREE_H
