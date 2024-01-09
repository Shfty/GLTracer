#include "kdTree.h"
//#define DEBUG

#include <queue>

const int MAX_OBJECTS_PER_LEAF = 1;

kdTree::kdTree( std::vector<Primitive*> primitives )
{
    BuildTree( primitives );
    true;
}

kdTree::~kdTree()
{
    delete m_rootNode;
}

void kdTree::BuildTree( std::vector<Primitive*> primitives )
{
    m_leafObjectsVector.clear();
#ifdef DEBUG
    std::cout << "Beginning kD Tree Build" << std::endl << std::endl;
#endif
    m_rootNode = BuildNode( primitives, 0 );
#ifdef DEBUG
    std::cout << "Constructing Tree Vector" << std::endl << std::endl;
#endif
    constructTreeVector();
}

kdNode* kdTree::BuildNode( std::vector<Primitive*> primitives, int depth )
{
    if( primitives.size() == 0 ) return constructLeafNode( primitives );

    bool samePos = true;
    Primitive* prevObject = primitives[ 0 ];
    for( int i = 0; i < primitives.size(); ++i )
    {
        if( primitives[ i ]->Position != prevObject->Position )
        {
            samePos = false;
            break;
        }
    }

    if( primitives.size() > MAX_OBJECTS_PER_LEAF && !samePos )
    {
        int axis = depth % 3;

        float splitPos = 0;
        for( int i = 0; i < primitives.size(); ++i )
        {
            splitPos += primitives[ i ]->Position[ axis ];
        }
        splitPos /= primitives.size();

        return constructBranchNode( primitives, axis, splitPos, depth );
    }
    else
    {
        return constructLeafNode( primitives );
    }
}

kdNode* kdTree::constructBranchNode( std::vector<Primitive*> primitives, int axis, float splitPos, int depth )
{
    std::vector<Primitive*> leftObjects;
    std::vector<Primitive*> rightObjects;
    for( int i = 0; i < primitives.size(); ++i )
    {
        if( primitives[ i ]->Position[ axis ] < splitPos )
        {
            leftObjects.push_back( primitives[ i ] );
        }
        else
        {
            rightObjects.push_back( primitives[ i ] );
        }
    }

    kdNode* node = new kdNode();
    node->Value = glm::vec4( 0.0f, axis, splitPos, -1.0f );

#ifdef DEBUG
    std::cout << "Branch Node" << std::endl;
    std::cout << "\tDepth: " << depth << std::endl;
    std::cout << "\tAxis: " << axis << std::endl;
    std::cout << "\tValue: " << node->Value[0] << ", " << node->Value[1] << std::endl << std::endl;
#endif

    node->LeftChild = BuildNode( leftObjects, depth + 1 );
    node->RightChild = BuildNode( rightObjects, depth + 1 );

    return node;
}

kdNode* kdTree::constructLeafNode( std::vector<Primitive*> primitives )
{
    kdNode* node = new kdNode();
    node->Value = glm::vec4( 1.0f, m_leafObjectsVector.size(), -1.0f, -1.0f );

    for( int i = 0; i < primitives.size(); ++i )
    {
        m_leafObjectsVector.push_back( primitives[ i ]->ID );
    }

    m_leafObjectsVector.push_back( -1 );

#ifdef DEBUG
    std::cout << "Leaf Node" << std::endl;
    std::cout << "\tValue: " << node->Value[0] << ", " << node->Value[1] << std::endl << std::endl;
#endif

    return node;
}

void kdTree::constructTreeVector()
{
    m_treeVector.clear();

    std::queue< kdNode* > nodeQueue;

    nodeQueue.push( m_rootNode );

    while( !nodeQueue.empty() )
    {
        kdNode* node = nodeQueue.front();
        nodeQueue.pop();

#ifdef DEBUG
        std::cout << "Value: " << node->Value[0] << ", " << node->Value[1] << std::endl << std::endl;
#endif
        m_treeVector.push_back( node->Value );

        if( node->LeftChild != NULL ) nodeQueue.push( node->LeftChild );
        if( node->RightChild != NULL ) nodeQueue.push( node->RightChild );
    }
}
