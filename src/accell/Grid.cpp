#include "accell/Grid.h"

#include "Ray.h"
#include "Collisions.h"
#include "Utility.h"

#define DRAW_OBJECT_CELLS
//#define DRAW_RAY_PATH

Ray testRay( glm::vec3( 5, 5, 5 ), glm::vec3( 0, 0, 1 ) );
std::vector< glm::vec3 > objectCells;
std::vector< glm::vec3 > hitCells;

Grid::Grid( std::vector< Primitive* > primitives, glm::vec3 p0, glm::vec3 p1, int subdivisions )
{
    m_subdivisions = subdivisions;
    m_arrayLength = subdivisions * subdivisions * subdivisions;
    m_grid = new int[ m_arrayLength ];
    m_p0 = p0;
    m_p1 = p1;

    m_cellSize = ( m_p1 - m_p0 ) / float( subdivisions );

    buildGrid( primitives );
}

Grid::~Grid()
{
    delete[] m_grid;
}

void Grid::Update( std::vector< Primitive* > primitives, const glm::vec3& camPos, const glm::vec3& camDir )
{
    //buildGrid( primitives );
    testRay.Origin = camPos;
    testRay.Direction = camDir;
#ifdef DRAW_RAY_PATH
    traverseGrid();
#endif
}

void Grid::traverseGrid()
{
    Ray ray = testRay;

    // Initialization
    IsectData gridIsectData;
    // Test grid bounds to see if ray intersects, return if not
    if( !Collisions::IsectAABB( ray, m_p0, m_p1, gridIsectData ) )
    {
        return;
    }
    else
    {
        // If the ray isn't inside the scene, advance to boundary
        if( !Collisions::AABBContainsPoint( ray.Origin, m_p0, m_p1 ) )
        {
            ray.Origin = gridIsectData.Position;
        }
    }

    glm::ivec3 step;
    step = sign( testRay.Direction );

    // Determine which cell the ray originated in
    glm::vec3 localPos = ray.Origin - m_p0;
    glm::ivec3 cell = glm::ivec3( localPos / m_cellSize );

    // Determine the shortest axis to a cell boundary
    glm::vec3 cellP0 = m_p0 + ( glm::vec3( cell ) * m_cellSize );
    glm::vec3 cellP1 = cellP0 + m_cellSize;

    glm::vec3 tMax( Collisions::FAR_PLANE );
    IsectData planeIsectData;

    for( int axis = 0; axis < 3; ++axis )
    {
        glm::vec3 planePos = glm::vec3( 0.0f );
        if( step[ axis ] == -1 )
        {
            planePos[ axis ] = cellP0[ axis ];
        }
        if( step[ axis ] == 1 )
        {
            planePos[ axis ] = cellP1[ axis ];
        }

        glm::vec3 planeNormal( 0.0f );
        planeNormal[ axis ] = -1.0f;

        if( Collisions::IsectPlane( ray, planePos, planeNormal, planeIsectData ) )
        {
            tMax[ axis ] = glm::distance( ray.Origin, planeIsectData.Position );
        }
    }

    glm::vec3 tDelta = m_cellSize * glm::vec3( step ) * ( 1.0f / ray.Direction );

    // Incrementation
    hitCells.clear();

    while( cell.x >= 0 && cell.x <= m_subdivisions &&
           cell.y >= 0 && cell.y <= m_subdivisions &&
           cell.z >= 0 && cell.z <= m_subdivisions )
    {
        hitCells.push_back( m_p0 + glm::vec3( cell ) * m_cellSize );

        float minTMax = min( tMax.x, min( tMax.y, tMax.z ) );

        if( minTMax == tMax.x )
        {
            tMax.x = tMax.x + tDelta.x;
            cell.x = cell.x + step.x;
        }
        else if( minTMax == tMax.y )
        {
            tMax.y = tMax.y + tDelta.y;
            cell.y = cell.y + step.y;
        }
        else if( minTMax == tMax.z )
        {
            tMax.z = tMax.z + tDelta.z;
            cell.z = cell.z + step.z;
        }
    }
}

void Grid::buildGrid( std::vector< Primitive* > primitives )
{
    memset( m_grid, -1, m_arrayLength * sizeof( int ) );
    objectCells.clear();

    for( int x = 0; x < m_subdivisions; ++x )
    {
        for( int y = 0; y < m_subdivisions; ++y )
        {
            for( int z = 0; z < m_subdivisions; ++z )
            {
                int idx = x + y * m_subdivisions + z * m_subdivisions * m_subdivisions;
                m_grid[ idx ] = m_cellObjectRefs.size();
                glm::vec3 pos0( m_p0.x + x * m_cellSize.x, m_p0.y + y * m_cellSize.y, m_p0.z + z * m_cellSize.z );
                glm::vec3 pos1 = pos0 + m_cellSize;
                bool objectCell = false;
                for( int i = 0; i < primitives.size(); ++i )
                {
                    glm::vec3 op0;
                    glm::vec3 op1;

                    // Determine Bounding Boxes for relevant shapes
                    switch( primitives[ i ]->Type )
                    {
                        case Primitive::Sphere:
                        case Primitive::Disc:
                        {
                            glm::vec3 dim = glm::vec3( 1.0f ) * max( primitives[ i ]->Scale.x, max( primitives[ i ]->Scale.y, primitives[ i ]->Scale.z ) );
                            op0 = primitives[ i ]->Position - dim;
                            op1 = primitives[ i ]->Position + dim;

                            break;
                        }
                        case Primitive::AABB:
                        {
                            glm::vec3 dim = glm::vec3( 0.5f ) * primitives[ i ]->Scale;
                            op0 = primitives[ i ]->Position - dim;
                            op1 = primitives[ i ]->Position + dim;

                            break;
                        }
                        default:
                            break;
                    }

                    // Compute grid intersections
                    switch( primitives[ i ]->Type )
                    {
                        case Primitive::Plane:
                        {
                            glm::vec3 cellP0 = m_p0 + ( glm::vec3( x, y, z ) * m_cellSize );
                            glm::vec3 cellP1 = cellP0 + m_cellSize;

                            if( Collisions::PlaneIntersectsAABB( primitives[ i ]->Position, glm::normalize( primitives[ i ]->Orientation * vec3( 0, -1, 0 ) ), cellP0, cellP1 ) )
                            {
                                m_cellObjectRefs.push_back( primitives[ i ]->ID );
                                objectCell = true;
                            }
                            break;
                        }
                        case Primitive::Sphere:
                        /* TODO: Fix for lower grid resolutions
                        {
                            glm::vec3 cellPos = m_p0 + ( glm::vec3( x, y, z ) * m_cellSize ) + m_cellSize * 0.5f;

                            if( Collisions::SphereContainsPoint( cellPos, primitives[ i ]->Position, primitives[ i ]->Orientation, primitives[ i ]->Scale * ( m_cellSize * 0.5f ) ) )
                            {
                                m_cellObjectRefs.push_back( primitives[ i ]->ID );
                                objectCell = true;
                            }
                            break;
                        }
                        */
                        case Primitive::Disc:
                        {
                            glm::vec3 cellP0 = m_p0 + ( glm::vec3( x, y, z ) * m_cellSize );
                            glm::vec3 cellP1 = cellP0 + m_cellSize;

                            if( Collisions::AABBContainsAABB( primitives[ i ]->Position - primitives[ i ]->Scale, primitives[ i ]->Position + primitives[ i ]->Scale, cellP0, cellP1 ) )
                            {
                                m_cellObjectRefs.push_back( primitives[ i ]->ID );
                                objectCell = true;
                            }
                            break;
                        }
                        case Primitive::AABB:
                        {
                            vec3 localPos0 = op0 - m_p0;
                            ivec3 cell0 = ivec3( localPos0 / m_cellSize );
                            vec3 localPos1 = op1 - m_p0;
                            ivec3 cell1 = ivec3( localPos1 / m_cellSize );

                            if( x >= cell0.x && x <= cell1.x &&
                                y >= cell0.y && y <= cell1.y &&
                                z >= cell0.z && z <= cell1.z )
                            {
                                if( x == cell0.x || y == cell0.y || z == cell0.z || x == cell1.x || y == cell1.y || z == cell1.z )
                                {
                                    m_cellObjectRefs.push_back( primitives[ i ]->ID );
                                    objectCell = true;
                                }
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
                if( objectCell )
                {
                    objectCells.push_back( m_p0 + glm::vec3( x, y, z ) * m_cellSize );
                }
                m_cellObjectRefs.push_back( -1 );
            }
        }
    }
}

void Grid::Draw()
{
#ifdef DRAW_OBJECT_CELLS
    glLineWidth( 1.0f );
    for( int i = objectCells.size() - 1; i >= 0; --i )
    {
        float distFactor = 30.0f / glm::distance( objectCells[ i ], testRay.Origin );
        glColor3f( 0, distFactor, 0 );
        drawCube( objectCells[ i ], objectCells[ i ] + m_cellSize );
    }
#endif

#ifdef DRAW_RAY_PATH
    for( int i = hitCells.size() - 1; i >= 0; --i )
    {
        float distFactor = 25.0f / glm::distance( hitCells[ i ], testRay.Origin );
        glLineWidth( distFactor );
        glColor3f( 0, 0, distFactor );
        drawCube( hitCells[ i ], hitCells[ i ] + m_cellSize );
    }
#endif
/*
    // Draw Ray
    glm::vec3 startpt = testRay.Origin;
    glm::vec3 endpt = startpt + testRay.Direction * Collisions::FAR_PLANE;

    glColor3f( 1, 1, 1 );
    glBegin( GL_LINES );
        glVertex3f( startpt.x, startpt.y, startpt.z );
        glVertex3f( endpt.x, endpt.y, endpt.z );
    glEnd();
*/
}

void Grid::drawCube( const glm::vec3& p0, const glm::vec3& p1 )
{
    // Bottom Face
    glBegin( GL_LINE_LOOP );
        glVertex3f( p0.x, p0.y, p0.z );
        glVertex3f( p1.x, p0.y, p0.z );
        glVertex3f( p1.x, p0.y, p1.z );
        glVertex3f( p0.x, p0.y, p1.z );
    glEnd();

    // Top Face
    glBegin( GL_LINE_LOOP );
        glVertex3f( p0.x, p1.y, p0.z );
        glVertex3f( p1.x, p1.y, p0.z );
        glVertex3f( p1.x, p1.y, p1.z );
        glVertex3f( p0.x, p1.y, p1.z );
    glEnd();

    glBegin( GL_LINES );
        glVertex3f( p0.x, p0.y, p0.z );
        glVertex3f( p0.x, p1.y, p0.z );

        glVertex3f( p1.x, p0.y, p0.z );
        glVertex3f( p1.x, p1.y, p0.z );

        glVertex3f( p1.x, p0.y, p1.z );
        glVertex3f( p1.x, p1.y, p1.z );

        glVertex3f( p0.x, p0.y, p1.z );
        glVertex3f( p0.x, p1.y, p1.z );
    glEnd();
}
