#include "TestScene.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "WorldClock.h"

Primitive* skySphere = new Primitive(
    Primitive::Sphere,
    glm::vec3( 0.0f, 0.0f, 0.0f ),
    glm::quat(),
    glm::vec3( 1000.0f )
);

Primitive* texturedSphere = new Primitive(
    Primitive::Sphere,
    glm::vec3( 45.0f, 0.0f, 65.0f ),
    glm::quat(),
    glm::vec3( 5.0f )
);

Primitive* boxWarpX = new Primitive(
    Primitive::AABB,
    glm::vec3( -10.0f, -2.5f, -70.0f ),
    glm::quat(),
    glm::vec3( 40.0f, 15.0f, 15.0f )
);

// Creates a scene of world objects and loads them into m_primitives
TestScene::TestScene( GLTracer* glTracer ) : Scene( glTracer )
{
    Primitive* sphere1 = new Primitive( Primitive::Sphere,
                                            glm::vec3( 37.5f, 0, -70.0f ),
                                            glm::quat(),
                                            glm::vec3( 5.0f, 5.0f, 5.0f ) );
    sphere1->Material.Type = ObjectMaterial::SolidColor;
    sphere1->Material.Color = glm::vec4( 0.8f, 0, 0, 1 );
    addPrimitive( sphere1 );

    Primitive* sphere2 = new Primitive( Primitive::Sphere,
                                            glm::vec3( -40.0f, 0, -10.0f ),
                                            glm::quat(),
                                            glm::vec3( 1.0f, 5.0f, 5.0f ) );
    sphere2->Material.Type = ObjectMaterial::SolidColor;
    sphere2->Material.Color = glm::vec4( 0, 0.8f, 0, 0.4f );
    addPrimitive( sphere2 );

    Primitive* sphere3 = new Primitive( Primitive::Sphere,
                                            glm::vec3( -30.0f, 0, -20.0f ),
                                            glm::quat(),
                                            glm::vec3( 5.0f, 5.0f, 1.0f ) );
    sphere3->Material.Type = ObjectMaterial::SolidColor;
    sphere3->Material.Color = glm::vec4( 0, 0, 0.8f, 0.9f );
    sphere3->Material.Reflection = 1.0f;
    addPrimitive( sphere3 );

    Primitive* sphere4 = new Primitive( Primitive::Sphere,
                                            glm::vec3( -40.0f, 0, -30.0f ),
                                            glm::quat(),
                                            glm::vec3( 5.0f ) );
    sphere4->Material.Type = ObjectMaterial::SolidColor;
    sphere4->Material.Color = glm::vec4( 0.8f, 0, 0.8f, 0.4f );
    sphere4->Material.RefractiveIndex = 0.66f;
    addPrimitive( sphere4 );

    ObjectMaterial planeWallMaterial;
    planeWallMaterial.Specular = 0.2f;
    planeWallMaterial.Diffuse = 0.8f;

    // Floor
    Primitive* plane = new Primitive(
        Primitive::Plane,
        glm::vec3( 0.0f,-5.0f, 0.0f ),
        glm::quat(),
        glm::vec3( 1.0 )
    );
    plane->Material = planeWallMaterial;
    plane->Material.Type = ObjectMaterial::SolidColor;
    plane->Material.Color = glm::vec4( 0.8f, 0.8f, 0.8f, 1.0f );
    addPrimitive( plane );

    // Sky
    skySphere->Material.Type = ObjectMaterial::SolidColor;
    skySphere->Material.Color = glm::vec4( 0.8f, 0.8f, 0.8f, -1.0f );
    skySphere->Material.Diffuse = 0.0f;
    skySphere->Material.Specular = 0.0f;
    skySphere->Material.Emissive = 1.0f;
    skySphere->Material.CastShadow = 0.0f;
    //addPrimitive( skySphere );

    // Walls
    Primitive* backWall = new Primitive(
        Primitive::Plane, //Primitive::ConvexPoly,
        glm::vec3( 20.0f,-50.0f,-80.0f ),
        glm::angleAxis( glm::radians( 90.0f ), glm::vec3( 1, 0, 0 ) ),
        glm::vec3( 125.0f )
    );
    backWall->Sides = 4.0f;
    backWall->Material = planeWallMaterial;
    backWall->Material.Type = ObjectMaterial::Texture;
    backWall->Material.Color = glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f );
    addPrimitive( backWall );

    Primitive* frontWall = new Primitive(
        Primitive::Plane, //Primitive::ConvexPoly,
        glm::vec3( 20.0f,-50.0f, 80.0f ),
        glm::angleAxis( glm::radians( -90.0f ), glm::vec3( 1, 0, 0 ) ),
        glm::vec3( 125.0f )
    );
    frontWall->Sides = 4.0f;
    frontWall->Material = planeWallMaterial;
    frontWall->Material.Type = ObjectMaterial::Texture;
    frontWall->Material.Color = glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f );
    addPrimitive( frontWall );

    Primitive* leftWall = new Primitive(
        Primitive::Plane, //Primitive::ConvexPoly,
        glm::vec3(-60.0f,-50.0f, 0.0f ),
        glm::angleAxis( glm::radians( 90.0f ), glm::vec3( 0, 0, 1 ) ),
        glm::vec3( 125.0f )
    );
    leftWall->Sides = 4.0f;
    leftWall->Material = planeWallMaterial;
    leftWall->Material.Type = ObjectMaterial::Texture;
    leftWall->Material.Color = glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );
    addPrimitive( leftWall );

    Primitive* rightWall = new Primitive(
        Primitive::Plane, //Primitive::ConvexPoly,
        glm::vec3( 100.0f,-50.0f, 0.0f ),
        glm::angleAxis( glm::radians( -90.0f ), glm::vec3( 0, 0, 1 ) ),
        glm::vec3( 125.0f )
    );
    rightWall->Sides = 4.0f;
    rightWall->Material = planeWallMaterial;
    rightWall->Material.Type = ObjectMaterial::Texture;
    rightWall->Material.Color = glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );
    addPrimitive( rightWall );

    // Portals
    // Sphere
    Primitive* spherePortalA = new Primitive(
        Primitive::Sphere,
        glm::vec3(-40.0f, 0.0f, 30.0f ),
        glm::angleAxis( glm::radians( 90.0f ), glm::vec3( 0, 1, 0 ) ),
        glm::vec3( 10.0f )
    );
    spherePortalA->Material.Type = ObjectMaterial::Portal;

    Primitive* spherePortalB = new Primitive(
        Primitive::Sphere,
        glm::vec3( 40.0f, 0.0f, 0.0f ),
        glm::angleAxis( glm::radians( 90.0f ), glm::vec3( 0, 1, 0 ) ),
        glm::vec3( 10.0f )
    );
    spherePortalB->Material.Type = ObjectMaterial::Portal;

    generatePortalTransform( spherePortalA, spherePortalB );

    addPrimitive( spherePortalA );
    addPrimitive( spherePortalB );

    // Poly
    Primitive* polyPortalA = new Primitive(
        Primitive::ConvexPoly,
        glm::vec3(-50.0f, 0.0f,-70.0f ),
        glm::angleAxis( glm::radians( 45.0f ), glm::vec3( 0, 1, 0 ) ) *
        glm::angleAxis( glm::radians( 90.0f ), glm::vec3( 1, 0, 0 ) ),
        glm::vec3( 7.0f )
    );
    polyPortalA->Sides = 5.0f;
    polyPortalA->Material.Type = ObjectMaterial::Portal;

    Primitive* polyPortalB = new Primitive(
        Primitive::ConvexPoly,
        glm::vec3( 20.0f, 0.0f, -50.0f ),
        glm::angleAxis( glm::radians( 180.0f ), glm::vec3( 0, 1, 0 ) ) *
        glm::angleAxis( glm::radians( 90.0f ), glm::vec3( 1, 0, 0 ) ),
        glm::vec3( 7.0f )
    );
    polyPortalB->Sides = 5.0f;
    polyPortalB->Material.Type = ObjectMaterial::Portal;

    generatePortalTransform( polyPortalA, polyPortalB );

    addPrimitive( polyPortalA );
    addPrimitive( polyPortalB );

    // Disc
    // Wall Portals
    Primitive* discPortalA = new Primitive(
        Primitive::Disc,
        glm::vec3(-20.0f, 0.0f, 79.9f ),
        glm::angleAxis( glm::radians( 90.0f ), glm::vec3( 1, 0, 0 ) ),
        glm::vec3( 5.0f )
    );
    discPortalA->Material.Type = ObjectMaterial::Portal;

    Primitive* discPortalB = new Primitive(
        Primitive::Disc,
        glm::vec3( 40.0f, 0.0f, -79.9f ),
        glm::angleAxis( glm::radians( 90.0f ), glm::vec3( 1, 0, 0 ) ),
        glm::vec3( 5.0f )
    );
    discPortalB->Material.Type = ObjectMaterial::Portal;

    generatePortalTransform( discPortalA, discPortalB );

    addPrimitive( discPortalA );
    addPrimitive( discPortalB );

    // Floor Portals
    Primitive* discPortalC = new Primitive(
        Primitive::Disc,
        glm::vec3( 80.0f,-4.9f,-60.0f ),
        glm::quat(),
        glm::vec3( 10.0f )
    );
    discPortalC->Material.Type = ObjectMaterial::Portal;

    Primitive* discPortalD = new Primitive(
        Primitive::Disc,
        glm::vec3( 80.0f, 20.0f, 60.0f ),
        glm::quat(),
        glm::vec3( 10.0f )
    );
    discPortalD->Material.Type = ObjectMaterial::Portal;

    generatePortalTransform( discPortalC, discPortalD );

    addPrimitive( discPortalC );
    addPrimitive( discPortalD );

    // Transparency Discs
    Primitive* transDiscA = new Primitive(
        Primitive::Disc,
        glm::vec3( 60.0f, 40.0f,-60.0f ),
        glm::angleAxis( glm::radians( -45.0f ), glm::vec3( 1, 0, 0 ) ),
        glm::vec3( 10.0f )
    );
    transDiscA->Material.Type = ObjectMaterial::SolidColor;
    transDiscA->Material.Color = glm::vec4( 1.0f, 0.0f, 0.0f, 0.4f );
    addPrimitive( transDiscA );

    Primitive* transDiscB = new Primitive(
        Primitive::Disc,
        glm::vec3( 60.0f, 30.0f,-50.0f ),
        glm::angleAxis( glm::radians( -45.0f ), glm::vec3( 1, 0, 0 ) ),
        glm::vec3( 10.0f )
    );
    transDiscB->Material.Type = ObjectMaterial::SolidColor;
    transDiscB->Material.Color = glm::vec4( 0.0f, 1.0f, 0.0f, 0.4f );
    addPrimitive( transDiscB );

    // Warps
    // Sphere
    Primitive* sphereWarp = new Primitive(
        Primitive::Sphere,
        glm::vec3( 45.0f, 0.0f, 65.0f ),
        glm::quat(),
        glm::vec3( 20.0f )
    );
    sphereWarp->Material.Type = ObjectMaterial::Spacewarp;
    sphereWarp->Material.Color = glm::vec4( 0.0f, 0.8f, 0.0f, 1.0f );
    sphereWarp->Material.PortalOffset = glm::vec3( 4.0f, 1.0f, 4.0f );
    addPrimitive( sphereWarp );

    //Box
    boxWarpX->Material.Type = ObjectMaterial::Spacewarp;
    boxWarpX->Material.Color = glm::vec4( 0.0f, 0.8f, 0.0f, 1.0f );
    boxWarpX->Material.PortalOffset = glm::vec3( 10.0f, 1.0f, 1.0f );
    addPrimitive( boxWarpX );

    // TARDIS Room
    Primitive* boxWarpY = new Primitive(
        Primitive::AABB,
        glm::vec3( 80.0f, 5.0f, 0.0f ),
        glm::quat(),
        glm::vec3( 14.9f, 19.9f, 14.9f )
    );
    boxWarpY->Material.Type = ObjectMaterial::Spacewarp;
    boxWarpY->Material.Color = glm::vec4( 0.0f, 0.8f, 0.0f, 1.0f );
    boxWarpY->Material.PortalOffset = glm::vec3( 1.0f, 0.1f, 1.0f );
    addPrimitive( boxWarpY );

    ObjectMaterial roomWallMaterial;
    roomWallMaterial.Type = ObjectMaterial::Texture;
    roomWallMaterial.Color = glm::vec4( 1.0f );
    roomWallMaterial.Diffuse = 0.0f;
    roomWallMaterial.Specular = 1.0f;
    roomWallMaterial.SpecularFactor = 64.0f;
    roomWallMaterial.Emissive = 0.2f;

    Primitive* roomWallLeft = new Primitive(
        Primitive::AABB,
        glm::vec3( 80.0f, 5.0f, -10.0f ),
        glm::quat(),
        glm::vec3( 15.0f, 20.0f, 5.0f )
    );
    roomWallLeft->Material = roomWallMaterial;
    addPrimitive( roomWallLeft );


    Primitive* roomWallRight = new Primitive(
        Primitive::AABB,
        glm::vec3( 80.0f, 5.0f, 10.0f ),
        glm::quat(),
        glm::vec3( 15.0f, 20.0f, 5.0f )
    );
    roomWallRight->Material = roomWallMaterial;
    addPrimitive( roomWallRight );

    Primitive* roomWallBack = new Primitive(
        Primitive::AABB,
        glm::vec3( 90.0f, 5.0f, 0.0f ),
        glm::quat(),
        glm::vec3( 5.0f, 20.0f, 25.0f )
    );
    roomWallBack->Material = roomWallMaterial;
    addPrimitive( roomWallBack );

    Primitive* roomWallFront = new Primitive(
        Primitive::AABB,
        glm::vec3( 70.0f, 15.0f, 0.0f ),
        glm::quat(),
        glm::vec3( 5.0f, 10.0f, 25.0f )
    );
    roomWallFront->Material = roomWallMaterial;
    addPrimitive( roomWallFront );

    Primitive* roomFrontPillarL = new Primitive(
        Primitive::AABB,
        glm::vec3( 70.0f, 2.5f, -8.5f ),
        glm::quat(),
        glm::vec3( 5.0f, 15.0f, 8.0f )
    );
    roomFrontPillarL->Material = roomWallMaterial;
    addPrimitive( roomFrontPillarL );

    Primitive* roomFrontPillarR = new Primitive(
        Primitive::AABB,
        glm::vec3( 70.0f, 2.5f, 8.5f ),
        glm::quat(),
        glm::vec3( 5.0f, 15.0f, 8.0f )
    );
    roomFrontPillarR->Material = roomWallMaterial;
    addPrimitive( roomFrontPillarR );


    Primitive* roomRoof = new Primitive(
        Primitive::AABB,
        glm::vec3( 82.5f, 17.5f, 0.0f ),
        glm::quat(),
        glm::vec3( 20.0f, 5.0f, 25.0f )
    );
    roomRoof->Material = roomWallMaterial;
    addPrimitive( roomRoof );

    // Compress Tunnel
    ObjectMaterial tunnelMaterial;
    tunnelMaterial.Type = ObjectMaterial::Texture;
    tunnelMaterial.Color = glm::vec4( 1.0f );
    tunnelMaterial.Diffuse = 0.0f;
    tunnelMaterial.Specular = 1.0f;
    tunnelMaterial.SpecularFactor = 64.0f;
    tunnelMaterial.Emissive = 0.2f;

    Primitive* cTunnelWarp = new Primitive(
        Primitive::AABB,
        glm::vec3( 0.0f, -1.25f, -11.0f ),
        glm::quat(),
        glm::vec3( 20.0f, 7.25f, 3.75f )
    );
    cTunnelWarp->Material.Type = ObjectMaterial::Spacewarp;
    cTunnelWarp->Material.PortalOffset = glm::vec3( 10.0f, 1.0f, 1.0f );
    addPrimitive( cTunnelWarp );

    Primitive* cTunnelLeft = new Primitive(
        Primitive::AABB,
        glm::vec3( 0.0f, -1.25f, -8.0f ),
        glm::quat(),
        glm::vec3( 20.0f, 7.5f, 2.0f )
    );
    cTunnelLeft->Material = tunnelMaterial;
    addPrimitive( cTunnelLeft );

    Primitive* cTunnelRight = new Primitive(
        Primitive::AABB,
        glm::vec3( 0.0f, -1.25f, -14.0f ),
        glm::quat(),
        glm::vec3( 20.0f, 7.5f, 2.0f )
    );
    cTunnelRight->Material = tunnelMaterial;
    addPrimitive( cTunnelRight );

    Primitive* cTunnelTop = new Primitive(
        Primitive::AABB,
        glm::vec3( 0.0f, 3.5f, -11.0f ),
        glm::quat(),
        glm::vec3( 20.0f, 2.0f, 8.0f )
    );
    cTunnelTop->Material = tunnelMaterial;
    addPrimitive( cTunnelTop );

    // Stretch Tunnel
    Primitive* sTunnelWarp = new Primitive(
        Primitive::AABB,
        glm::vec3( 0.0f, -1.25f, -21.0f ),
        glm::quat(),
        glm::vec3( 2.0f, 7.25f, 3.75f )
    );
    sTunnelWarp->Material.Type = ObjectMaterial::Spacewarp;
    sTunnelWarp->Material.PortalOffset = glm::vec3( 0.1f, 1.0f, 1.0f );
    addPrimitive( sTunnelWarp );

    Primitive* sTunnelLeft = new Primitive(
        Primitive::AABB,
        glm::vec3( 0.0f, -1.25f, -18.0f ),
        glm::quat(),
        glm::vec3( 2.0f, 7.5f, 2.0f )
    );
    sTunnelLeft->Material = tunnelMaterial;
    addPrimitive( sTunnelLeft );

    Primitive* sTunnelRight = new Primitive(
        Primitive::AABB,
        glm::vec3( 0.0f, -1.25f, -24.0f ),
        glm::quat(),
        glm::vec3( 2.0f, 7.5f, 2.0f )
    );
    sTunnelRight->Material = tunnelMaterial;
    addPrimitive( sTunnelRight );

    Primitive* sTunnelTop = new Primitive(
        Primitive::AABB,
        glm::vec3( 0.0f, 3.5f, -21.0f ),
        glm::quat(),
        glm::vec3( 2.0f, 2.0f, 8.0f )
    );
    sTunnelTop->Material = tunnelMaterial;
    addPrimitive( sTunnelTop );

    // Misc objects
    Primitive* triangle = new Primitive(
        Primitive::ConvexPoly,
        glm::vec3( -40.0f, 0.0f, 60.0f ),
        glm::angleAxis( glm::radians( 95.0f ), glm::vec3( 1, 0, 0 ) ),
        glm::vec3( 5.0f, 1.0f, 10.0f )
    );
    triangle->Sides = 3.0f;
    triangle->Material.Type = ObjectMaterial::SolidColor;
    triangle->Material.Color = glm::vec4( 0.0f, 0.0f, 0.8f, 0.9f );
    triangle->Material.Reflection = 1.0f;
    addPrimitive( triangle );

    // Textured Sphere Setup
    texturedSphere->Material.Type = ObjectMaterial::Texture;
    texturedSphere->Material.Color = glm::vec4( glm::vec3( 1.0f ), 1.0f );
    texturedSphere->Material.Diffuse = 1.0f;
    texturedSphere->Material.Specular = 0.0f;
    addPrimitive( texturedSphere );
}

void TestScene::Update( glm::vec4 skyColor )
{
    Scene::Update();

    float clock = WorldClock::Instance()->TotalTime();

    // Sky color
    skySphere->Material.Color = skyColor;
    updatePrimitive( skySphere );

    // Sphere position
    texturedSphere->Position.x = 45.0f + glm::sin( float( clock ) ) * 20.0f;
    texturedSphere->Position.y = 20.0f - glm::cos( float( clock ) ) * 20.0f;
    updatePrimitive( texturedSphere );

    // Box warp
    boxWarpX->Material.PortalOffset.x = 10.0f * ( sin( clock * 0.5f ) + 1.0f ) / 2.0f;
    updatePrimitive( boxWarpX );
}
