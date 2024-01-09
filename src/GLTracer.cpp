#include "GLTracer.h"

#include <iostream>
#include <sstream>
#include <stack>
#include "TestScene.h"
#include "WorldClock.h"
#include "Controls.h"
#include "Collisions.h"

#define ACC_NONE 0
#define ACC_GRID 1
//#define ACC_OCTREE 2
#define ACC_KDTREE 3

#define ACCELL_STRUCTURE ACC_GRID
#define RENDER_DEBUG
#define RENDER_CROSSHAIR

const float FOV = 90.0f;
const float SKYLIGHT_ROTATE_PER_SEC = 0.01f;
const int INFO_PACKET_SIZE = 24;
const float AMBIENT_INTENSITY = 0.2f;
const int GRID_RESOLUTION = 20;

int prevWorldClock;

glm::vec3 skyLightDirection = glm::vec3(0, 1, 0);
glm::vec4 dayColor = glm::vec4(0, 1, 1, 1);
glm::vec4 nightColor = glm::vec4(0, 0, 0.5, 1);

std::string windowTitle = "GLTracer";
glm::vec2 windowBounds = glm::vec2( 1280.0f, 720.0f );
glm::vec2 renderBounds = glm::round( windowBounds / 1.5f );
glm::vec2 viewportBounds;
glm::vec2 viewportPadding;
int uniform_WindowSize = 0;
int uniform_FOV = 0;

GLuint s_raytracerProgram = 0;

TestScene* scene;

GLFWwindow* Utility::MainWindow;
glm::ivec2 Controls::MouseOrigin;

// Perform initial setup, fetch OpenGL uniform IDs, setup initial uniform/world states
GLTracer::GLTracer()
{
    // Initialize OpenGL and open a window
    initGL();

    setupRenderTexture();

    setupVertexBuffer();

    m_camera = new Camera();
    m_projectionMatrix = glm::perspective( FOV, windowBounds.x / windowBounds.y, 0.1f, 1000.0f );

    // Send primitives into the info texture
    scene = new TestScene( this );
#if ACCELL_STRUCTURE == ACC_GRID
    m_grid = new Grid( scene->GetObjects(), glm::vec3( -100, -100, -100 ), glm::vec3( 100, 100, 100 ), GRID_RESOLUTION );
#endif
#if ACCELL_STRUCTURE == ACC_KDTREE
    m_kdTree = new kdTree( scene->GetObjects() );
#endif

    compileShaders();

    generateObjectInfoTex();
    bufferPrimitives( scene->GetObjects() );

#if ACCELL_STRUCTURE == ACC_GRID
    generateGridTex();
    bufferGrid();
#endif

#if ACCELL_STRUCTURE == ACC_KDTREE
    generateKDTreeTex();
    bufferKDTree();
#endif

    compileShaders();
}

// Deallocate heap and OpenGL objects
GLTracer::~GLTracer()
{
    delete scene;

    delete m_camera;

    delete m_basicVS;
    delete m_basicFS;
    delete m_raytracerFS;

    terminateGL();
}

void GLTracer::terminateGL()
{
    if( m_raytracerProgram != 0 )
    {
        glDeleteProgram( m_raytracerProgram );
    }

    if( m_basicProgram != 0 )
    {
        glDeleteProgram( m_basicProgram );
    }

    glfwDestroyWindow( m_window );

    glDeleteBuffers( 1, &m_vertexBuffer );
    glDeleteBuffers( 1, &m_texCoordBuffer );

    glDeleteTextures( 1, &m_objectInfoTex );
    glDeleteBuffers( 1, &m_objectInfoTBO );

    glDeleteTextures( 1, &m_accellStructureTex );
    glDeleteBuffers( 1, &m_accellStructureTBO );

    glDeleteTextures( 1, &m_objectRefTex );
    glDeleteBuffers( 1, &m_objectRefTBO );

    glDeleteTextures( 1, &m_screenColorTexture );
    glDeleteTextures( 1, &m_screenDepthTexture );
    glDeleteFramebuffers( 1, &m_framebuffer );

    glfwTerminate();
}

// Performs per-frame changes to the world state
void GLTracer::Update()
{
    // Raytracer program needs to be active to receive data
    glUseProgram( m_raytracerProgram );

    // Camera
    m_camera->Update( scene->GetObjects() );

    // Sky light direction
    float deltaSkyLightAngle = SKYLIGHT_ROTATE_PER_SEC * WorldClock::Instance()->DeltaTime();
    skyLightDirection = glm::normalize( glm::vec3( glm::rotate( deltaSkyLightAngle, glm::vec3( 0, 0, 1 ) ) * glm::vec4( skyLightDirection, 1.0f ) ) );

    glm::vec4 skyColor = glm::lerp( dayColor, nightColor, ( -skyLightDirection.y + 1 ) / 2 );
    scene->Update( skyColor );
#if ACCELL_STRUCTURE == ACC_GRID
    m_grid->Update( scene->GetObjects(), m_camera->GetPosition(), glm::vec3( m_camera->GetRotation() * glm::vec4( 0, 0, -1, 0 ) ) );
#endif

    // Window title info readout
    static int ticks = 0;
    static int frames = 0;
    ticks += WorldClock::Instance()->ElapsedTime();
    frames++;
    if( ticks > 1000 )
    {
        std::stringstream ss;
        ss << windowTitle << std::string( " | FPS: " ) << frames;
        ss << " | Internal Resolution: " << renderBounds.x << "x" << renderBounds.y;
        ss << " | Window Resolution: " << windowBounds.x << "x" << windowBounds.y;
        glfwSetWindowTitle( m_window, ss.str().c_str() );
        ticks -= 1000;
        frames = 0;
    }

    // Unlock mouse / Quit
    static bool prevMenu = false;

    if( Controls::Menu() && !prevMenu )
    {
        if( Controls::GetMouseLock() )
        {
            Controls::SetMouseLock( false );
        }
        else
        {
            terminateGL();
            exit( 0 );
        }
    }

    prevMenu = Controls::Menu();

    // Lock mouse
    static bool prevLeftClick = false;

    if( Controls::LeftClick() && !prevLeftClick )
    {
        Controls::SetMouseLock( true );
        Controls::ResetMousePos();
    }

    prevLeftClick = Controls::LeftClick();
}

// DEBUG: kD Tree Walking
void GLTracer::walkKDTree()
{
    std::vector< int > objectsToCheck;

    // GLSL samplers
    std::vector< glm::vec4 > treeVector = m_kdTree->GetTreeVector();
    std::vector< int > leafObjectsVector = m_kdTree->GetLeafObjectsVector();

    // GLSL arguments
    Ray viewRay(
                m_camera->GetPosition(),
                glm::normalize( glm::vec3( m_camera->GetRotation() * glm::vec4( 0, 0, -1, 0 ) ) )
                );
    IsectData isectData;

    // Tree traversal
#define leftChild( i ) ( 2 * i + 1 )
#define rightChild( i ) ( 2 * i + 2 )
#define parent( i ) ( ( i - 1 ) / 2 )
    const int MAX_STACK_SIZE = 500;

    // Current position in the kD Tree array
    int index = 0;

    // Stack
    int traversalStack[ MAX_STACK_SIZE ];
    for( int i = 0; i < MAX_STACK_SIZE; ++i ) traversalStack[ i ] = -1;
    int stackPointer = 0;
    traversalStack[ stackPointer ] = 0; // Root node

    // Vector of objects to traverse
    int leafObjects[ MAX_STACK_SIZE ];
    int leafObjectIndex = 0;

    // Current node
    glm::vec4 node;

    for( int i = 0; i < MAX_STACK_SIZE; ++i )
    {
        if( stackPointer < 0 ) break;

        // Pop off the top stack element
        index = traversalStack[ stackPointer ];
        node = treeVector[ index ];
        stackPointer--;

        // branch node stores 0.0, axis, splitPosition
        bool leaf = node[ 0 ] == 1;
        int axis = int( node[ 1 ] );
        float splitPosition = node[ 2 ];

        // When encountering a leaf node, check it's objects
        if( leaf )
        {
            // leaf node stores 1.0, object vector index pointer, -1.0
            int index = int( node[ 1 ] );
            while( leafObjectsVector[ index ] != -1 )
            {
                leafObjects[ leafObjectIndex ] = leafObjectsVector[ index ];
                leafObjectIndex++;
                index++;
            }
            continue;
        }

        // Otherwise, check the ray against the splitting plane
        // and add the intersected child nodes to the stack
        bool tl = false;
        bool tr = false;
        if( viewRay.Origin[ axis ] < splitPosition )
        {
            tl = true;
            if( viewRay.Direction[ axis ] > 0 ) tr = true;

        }
        else
        {
            tr = true;
            if( viewRay.Direction[ axis ] < 0 ) tl = true;
        }

        int leftChildIndex = leftChild( index );
        bool leftChildValid = leftChildIndex < treeVector.size();
        if( tl && leftChildValid )
        {
            // Push left child onto stack
            stackPointer++;
            traversalStack[ stackPointer ] = leftChildIndex;
        }

        int rightChildIndex = rightChild( index );
        bool rightChildValid = rightChildIndex < treeVector.size();
        if( tr && rightChildValid )
        {
            // Push right child onto stack
            stackPointer++;
            traversalStack[ stackPointer ] = rightChildIndex;
        }
    }

    std::cout << "Leaf node objects:" << std::endl;
    for( int i = 0; i < leafObjectIndex; ++i )
    {
        std::cout << leafObjects[ i ] << ", ";
    }
    std::cout << std::endl << std::endl;
}

// Clear the screen, draw the screen quad and swap buffers
void GLTracer::Draw()
{
    // Update world objects and prepare kD tree
    bufferPrimitives( scene->GetUpdatedObjects() );
#if ACCELL_STRUCTURE == ACC_GRID
    //bufferGrid(); // Currently broken
#endif

#if ACCELL_STRUCTURE == ACC_KDTREE
    m_kdTree->BuildTree( scene->GetObjects() );
    walkKDTree();
    bufferKDTree();
#endif

    // Update uniforms
    //Camera
    glm::vec3 cameraPos = m_camera->GetPosition();
    glUniform3f( m_uniform_CameraPos, cameraPos.x, cameraPos.y, cameraPos.z );
    glUniformMatrix4fv( m_uniform_CameraRot, 1, GL_FALSE, glm::value_ptr( m_camera->GetRotation() ) );
    glUniformMatrix4fv( m_uniform_CameraRotInverse, 1, GL_FALSE, glm::value_ptr( glm::inverse( m_camera->GetRotation() ) ) );

    //Sky
    glUniform3f( m_uniform_SkyLightDirection, skyLightDirection.x, skyLightDirection.y, skyLightDirection.z );

    // Render to internal texture
    glBindFramebuffer( GL_FRAMEBUFFER, m_framebuffer );
    glViewport( 0, 0, renderBounds.x, renderBounds.y );
    glUseProgram( m_raytracerProgram );

    glEnable( GL_DEPTH_TEST );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glDrawArrays( GL_QUADS, 0, 4 );

    // Draw debug components
    glUseProgram( 0 );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glMultMatrixf( glm::value_ptr( m_projectionMatrix ) );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glMultMatrixf( glm::value_ptr( glm::inverse( m_camera->GetRotation() ) ) );
    glTranslatef( -m_camera->GetPosition().x, -m_camera->GetPosition().y, -m_camera->GetPosition().z );

    glBegin( GL_TRIANGLES );
        glVertex3f( 0.0, 20.0, -10.0 );
        glVertex3f( 10.0, 0.0, -10.0 );
        glVertex3f(-10.0, 0.0, -10.0 );
    glEnd();
#if ACCELL_STRUCTURE == ACC_GRID
#ifdef RENDER_DEBUG
    m_grid->Draw();
#endif
#endif

    glDisable( GL_DEPTH_TEST );

    // Draw to screen
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glViewport( viewportPadding.x, viewportPadding.y, viewportBounds.x, viewportBounds.y );
    glUseProgram( m_basicProgram );

    glClear( GL_COLOR_BUFFER_BIT );
    glDrawArrays( GL_QUADS, 0, 4 );

#ifdef RENDER_CROSSHAIR
    glUseProgram( 0 );
    glLoadIdentity();
    glColor3f( 1, 1, 1 );
    glLineWidth( 1.0f );
    glBegin( GL_LINES );
        glVertex3f(-0.025f, 0.0f,-1.0f );
        glVertex3f( 0.025f, 0.0f,-1.0f );

        glVertex3f( 0.0f,-0.025f,-1.0f );
        glVertex3f( 0.0f, 0.025f,-1.0f );
    glEnd();
#endif

    glfwSwapBuffers( m_window );
    glfwPollEvents();

    if( !glfwGetWindowAttrib( m_window, GLFW_VISIBLE ) )
    {
        glfwShowWindow( m_window );
    }
}

// Buffers the entire contents of m_primitives to the info texture
void GLTracer::BufferPrimitive( const Primitive* Primitive, const int idx )
{
    // Set buffer data
    glBindBuffer( GL_TEXTURE_BUFFER, m_objectInfoTBO );
    glm::vec4* p = ( glm::vec4* ) glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY );

    // Shared parameters
    // Info Cell
    p[ ( idx * INFO_PACKET_SIZE ) + 0 ] = glm::vec4( Primitive->ID, // ID
                                                  ( GLfloat )Primitive->Type,
                                                  -1.0f,
                                                  -1.0f );

    // World Matrix
    glm::mat4 transMatrix = glm::translate( Primitive->Position );
    glm::mat4 rotMatrix = glm::mat4_cast( Primitive->Orientation );
    glm::mat4 scaleMatrix = glm::scale( Primitive->Scale );


    glm::mat4 worldMatrix = transMatrix * rotMatrix * scaleMatrix;
    for( uint16_t o = 0; o < 4; o++ )
    {
        p[ ( idx * INFO_PACKET_SIZE ) + 1 + o ] = worldMatrix[ o ];
    }

    // Inverse World Matrix
    glm::mat4 inverseWorldMatrix = glm::inverse( worldMatrix );
    for( uint16_t o = 0; o < 4; o++ )
    {
        p[ ( idx * INFO_PACKET_SIZE ) + 5 + o ] = inverseWorldMatrix[ o ];
    }

    // Normal Matrix
    glm::mat4 normalMatrix = glm::inverseTranspose( worldMatrix );
    for( uint16_t o = 0; o < 4; o++ )
    {
        p[ ( idx * INFO_PACKET_SIZE ) + 9 + o ] = normalMatrix[ o ];
    }

    // Material
    // Type]#
    p[ ( idx * INFO_PACKET_SIZE ) + 13 ] = glm::vec4( ( GLfloat )Primitive->Material.Type,
                                                  -1.0f,
                                                  -1.0f,
                                                  -1.0f );

    // Color
    p[ ( idx * INFO_PACKET_SIZE ) + 14 ] = Primitive->Material.Color;

    // Lighting
    p[ ( idx * INFO_PACKET_SIZE ) + 15 ] = glm::vec4( Primitive->Material.Diffuse,
                                                     Primitive->Material.Specular,
                                                     Primitive->Material.SpecularFactor,
                                                     Primitive->Material.Emissive );

    // Effects
    p[ ( idx * INFO_PACKET_SIZE ) + 16 ] = glm::vec4( Primitive->Material.Reflection,
                                                     Primitive->Material.RefractiveIndex,
                                                     Primitive->Material.CastShadow,
                                                     -1.0 );


    // Portal Offset / Spacewarp Factor
    p[ ( idx * INFO_PACKET_SIZE ) + 17 ] = glm::vec4( Primitive->Material.PortalOffset,
                                                  0.0f );

    // Portal Rotation AxisAngle
    p[ ( idx * INFO_PACKET_SIZE ) + 18 ] = glm::vec4( Primitive->Material.PortalAxis,
                                                      Primitive->Material.PortalAngle );

    // Object-specific parameters
    // Sides ( ConvexPoly )
    p[ ( idx * INFO_PACKET_SIZE ) + 19 ] = glm::vec4( Primitive->Sides,
                                                     -1.0f,
                                                     -1.0f,
                                                     -1.0f );

    glUnmapBuffer( GL_TEXTURE_BUFFER );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
}

// Setup GLFW and GLEW to obtain an >= OpenGL 3.1 context and open a window
void GLTracer::initGL()
{
    // Init GLFW
    if( glfwInit() == GL_FALSE )
    {
        std::cerr << "Failed to initialize OpenGL, quitting." << std::endl;
        exit( -1 );
    }

    // Create a hidden window and setup
    glfwWindowHint( GLFW_VISIBLE, GL_FALSE );
    m_window = glfwCreateWindow( windowBounds.x, windowBounds.y, "", NULL, NULL );
    Utility::MainWindow = m_window;
    glfwMakeContextCurrent( m_window );

    // Setup window and show
    const GLFWvidmode* vm = glfwGetVideoMode( glfwGetPrimaryMonitor() );
    glfwSetWindowPos( m_window, vm->width / 2 - windowBounds.x / 2, vm->height / 2 - windowBounds.y / 2 );
    glfwSetWindowTitle( m_window, windowTitle.c_str() );

    // Setup window callbacks
    callbackResizeWindow( 0, windowBounds.x, windowBounds.y );
    glfwSetWindowSizeCallback( m_window, callbackResizeWindow );
    glfwSetWindowFocusCallback( m_window, callbackFocusWindow );
    glfwSetWindowCloseCallback( m_window, callbackCloseWindow );
    glfwSetCursorPosCallback( m_window, Controls::UpdateMousePos );

    std::string versionString( ( const char* )glGetString ( GL_VERSION ) );
    float glVersion = ::atof(versionString.substr(0,3).c_str());
    std::cout << "OpenGL Version: " << glVersion << std::endl;
    if( glVersion < 3.1f )
    {
        std::cerr << "OpenGL version 3.1 required, shutting down." << std::endl;
        glfwTerminate();
        exit( -1 );
    }

    // Init GLEW
    GLenum err = glewInit();
    if ( GLEW_OK != err )
    {
        std::cerr << "GLEW Init Error: " << glewGetErrorString( err ) << std::endl;
        glfwTerminate();
        exit( -1 );
    }
    std::cout << "GLEW Init Success, Using Version " << glewGetString( GLEW_VERSION ) << std::endl << std::endl;

    // Set clear color
    glClearColor( 1.0f, 0.0f, 0.0f, 1.0f );
}

// Generate and setup framebuffer, render texture
void GLTracer::setupRenderTexture()
{
    // Color Texture
    glActiveTexture( GL_TEXTURE0 );
    glGenTextures( 1, &m_screenColorTexture );
    glBindTexture( GL_TEXTURE_2D, m_screenColorTexture );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, renderBounds.x, renderBounds.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL );

    // Depth Texture
    glActiveTexture( GL_TEXTURE1 );
    glGenTextures( 1, &m_screenDepthTexture );
    glBindTexture( GL_TEXTURE_2D, m_screenDepthTexture );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, renderBounds.x, renderBounds.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL );

    // Generate framebuffer and bind color texture
    glGenFramebuffers( 1, &m_framebuffer );
    glBindFramebuffer( GL_FRAMEBUFFER, m_framebuffer );
    glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_screenColorTexture, 0 );
    glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_screenDepthTexture, 0 );

    if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
    {
        std::cerr << "Unable to complete Framebuffer setup, quitting...";
        exit( -1 );
    }
}

// Generate the screen quad vertex buffer and load in vertices
void GLTracer::setupVertexBuffer()
{
    glGenBuffers( 1, &m_vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, m_vertexBuffer );

    // Vertices - Location 0
    glm::vec3 quadVertices[4] = {
        glm::vec3(-1.0, 1.0, 0.0 ),
        glm::vec3( 1.0, 1.0, 0.0 ),
        glm::vec3( 1.0,-1.0, 0.0 ),
        glm::vec3(-1.0,-1.0, 0.0 )
    };

    glBufferData( GL_ARRAY_BUFFER, 4 * sizeof( glm::vec3 ), quadVertices, GL_STATIC_DRAW );

    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 );
    glEnableVertexAttribArray( 0 );

    // Texture Coordinates - Location 1
    glGenBuffers( 1, &m_texCoordBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, m_texCoordBuffer );

    glm::vec2 quadTexCoords[4] = {
        glm::vec2( 0.0, 1.0 ),
        glm::vec2( 1.0, 1.0 ),
        glm::vec2( 1.0, 0.0 ),
        glm::vec2( 0.0, 0.0 )
    };

    glBufferData( GL_ARRAY_BUFFER, 4 * sizeof( glm::vec2 ), quadTexCoords, GL_STATIC_DRAW );

    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, 0 );
    glEnableVertexAttribArray( 1 );
}

// Attempt to compile and link shaders,
// bind the correct vertex/fragment variable names,
// send the relevant info logs to standard output
void GLTracer::compileShaders()
{
    // Individual Vertex/Fragment Shaders
    if( m_basicVS != 0 )
    {
        delete m_basicVS;
    }

    if( m_basicFS != 0 )
    {
        delete m_basicFS;
    }

    if( m_raytracerFS != 0 )
    {
        delete m_raytracerFS;
    }

    m_basicVS = new ShaderProgram( std::string("shaders/BasicVert.vert"), GL_VERTEX_SHADER );
    m_basicVS->Compile();
    m_basicFS = new ShaderProgram( std::string("shaders/BasicFrag.frag"), GL_FRAGMENT_SHADER );
    m_basicFS->Compile();

    m_raytracerFS = new ShaderProgram( std::string("shaders/Raytracer.frag"), GL_FRAGMENT_SHADER );
    std::vector< std::vector< std::string > > rtConstants = {
        { STR_BOOL, "DISABLE_LIGHTING", STR_FALSE },
        { STR_BOOL, "DISABLE_SHADOWS", STR_FALSE },
        { STR_BOOL, "LOW_ACCURACY_MODE", STR_FALSE },
        { STR_BOOL, "DRAW_DEPTH_BUFFER", STR_FALSE }
    };
    m_raytracerFS->Compile( &rtConstants );

    // Basic Texture Program
    if( m_basicProgram != 0 )
    {
        glDeleteProgram( m_basicProgram );
    }

    m_basicProgram = glCreateProgram();
    glBindAttribLocation(m_basicProgram, 0, "vertex");
    glBindFragDataLocation(m_basicProgram, 0, "fragColor");
    glAttachShader( m_basicProgram, m_basicVS->GetID() );
    glAttachShader( m_basicProgram, m_basicFS->GetID() );
    glLinkProgram( m_basicProgram );

    GLint basicLinked = GL_FALSE;
    glGetProgramiv( m_basicProgram, GL_LINK_STATUS, &basicLinked );
    std::cout << "Program " << m_basicProgram << " link status: " << (basicLinked ? "GL_TRUE" : "GL_FALSE") << std::endl;

    GLchar basicInfoLog[ GL_INFO_LOG_LENGTH ] = { 0 };
    glGetProgramInfoLog( m_basicProgram, GL_INFO_LOG_LENGTH, NULL, basicInfoLog );
    std::cout << std::endl << "Basic Program info log:" << std::endl << basicInfoLog << std::endl;

    // Raytracer Program
    if( m_raytracerProgram != 0 )
    {
        glDeleteProgram( m_raytracerProgram );
    }

    m_raytracerProgram = glCreateProgram();
    s_raytracerProgram = m_raytracerProgram;
    glAttachShader( m_raytracerProgram, m_basicVS->GetID() );
    glAttachShader( m_raytracerProgram, m_raytracerFS->GetID() );
    glLinkProgram( m_raytracerProgram );

    GLint raytracerLinked = GL_FALSE;
    glGetProgramiv( m_raytracerProgram, GL_LINK_STATUS, &raytracerLinked );
    std::cout << "Program " << m_raytracerProgram << " link status: " << (raytracerLinked ? "GL_TRUE" : "GL_FALSE") << std::endl;

    GLchar raytracerInfoLog[ GL_INFO_LOG_LENGTH ] = { 0 };
    glGetProgramInfoLog( m_raytracerProgram, GL_INFO_LOG_LENGTH, NULL, raytracerInfoLog );
    std::cout << std::endl << "Raytracer Program info log:" << std::endl << raytracerInfoLog << std::endl;

    setupUniforms();
}

void GLTracer::setupUniforms()
{
    // Retrieve Basic uniform IDs and set screen texture location
    glUseProgram( m_basicProgram );
    glUniform1i( glGetUniformLocation( m_basicProgram, "ScreenTextureSampler" ), 0 );

    // Retrieve Raytracer uniform IDs and set uniforms
    glUseProgram( m_raytracerProgram );

    uniform_WindowSize = glGetUniformLocation( m_raytracerProgram, "WindowSize" );
    uniform_FOV = glGetUniformLocation( m_raytracerProgram, "FOV" );
    glUniform2f( uniform_WindowSize, renderBounds.x, renderBounds.y );
    glUniform1f( uniform_FOV, glm::radians( FOV ) );

    m_uniform_CameraPos = glGetUniformLocation( m_raytracerProgram, "CameraPos" );
    m_uniform_CameraRot = glGetUniformLocation( m_raytracerProgram, "CameraRot" );
    m_uniform_CameraRotInverse = glGetUniformLocation( m_raytracerProgram, "CameraRotInverse" );

    m_uniform_AmbientIntensity = glGetUniformLocation( m_raytracerProgram, "AmbientIntensity" );
    m_uniform_SkyLightColor = glGetUniformLocation( m_raytracerProgram, "SkyLightColor" );
    m_uniform_SkyLightDirection = glGetUniformLocation( m_raytracerProgram, "SkyLightDirection" );

    glUniform1f( m_uniform_AmbientIntensity, AMBIENT_INTENSITY );
    glUniform4f( m_uniform_SkyLightColor, 1.0, 1.0, 1.0, 1.0 );

    glUniform1i( glGetUniformLocation( m_raytracerProgram, "GridSubdivisions" ), m_grid->GetSubdivisions() );
    glm::vec3 p0 = m_grid->GetMinBound();
    glUniform3f( glGetUniformLocation( m_raytracerProgram, "GridMinBound" ), p0.x, p0.y, p0.z );
    glm::vec3 p1 = m_grid->GetMaxBound();
    glUniform3f( glGetUniformLocation( m_raytracerProgram, "GridMaxBound" ), p1.x, p1.y, p1.z );
    glm::vec3 cs = m_grid->GetCellSize();
    glUniform3f( glGetUniformLocation( m_raytracerProgram, "GridCellSize" ), cs.x, cs.y, cs.z );

    glUniform1i( glGetUniformLocation( m_raytracerProgram, "PrimitiveSampler" ), 2 );
    glUniform1i( glGetUniformLocation( m_raytracerProgram, "AccellStructureSampler" ), 3 );
    glUniform1i( glGetUniformLocation( m_raytracerProgram, "ObjectRefSampler" ), 4 );
}

// Performs initial setup of the object info texture and it's buffer
void GLTracer::generateObjectInfoTex()
{
    glGenBuffers( 1, &m_objectInfoTBO );
    glBindBuffer( GL_TEXTURE_BUFFER, m_objectInfoTBO );

    int bufferSize = scene->GetObjects().size() * INFO_PACKET_SIZE * sizeof( glm::vec4 );
    glBufferData( GL_TEXTURE_BUFFER, bufferSize, 0, GL_DYNAMIC_DRAW );

    // Create object info texture & bind it to the buffer
    glGenTextures( 1, &m_objectInfoTex );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_BUFFER, m_objectInfoTex );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_objectInfoTBO );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
}

// Buffers the provided set of world objects to the info texture
void GLTracer::bufferPrimitives( std::vector< Primitive* > primitives )
{
    // Load world objects into info texture
    for( uint16_t i = 0; i < primitives.size(); i++ )
    {
        BufferPrimitive( primitives[ i ], i );
    }

    glUseProgram( m_raytracerProgram );
    glUniform1i( glGetUniformLocation( m_raytracerProgram, "ObjectCount" ), scene->GetObjects().size() );
    glUniform1i( glGetUniformLocation( m_raytracerProgram, "ObjectInfoSize" ), INFO_PACKET_SIZE );
}

// Performs initial setup of the grid texture and it's buffer
void GLTracer::generateGridTex()
{
    // Generate tree texture
    glGenBuffers( 1, &m_accellStructureTBO );
    glBindBuffer( GL_TEXTURE_BUFFER, m_accellStructureTBO );

    int gridBufferSize = m_grid->GetGridArrayLength() * 4 * sizeof( float );
    glBufferData( GL_TEXTURE_BUFFER, gridBufferSize, 0, GL_DYNAMIC_DRAW );

    // Create accell structure texture & bind it to the buffer
    glGenTextures( 1, &m_accellStructureTex );
    glActiveTexture( GL_TEXTURE3 );
    glBindTexture( GL_TEXTURE_BUFFER, m_accellStructureTex );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_accellStructureTBO );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    // Generate object reference buffer
    glGenBuffers( 1, &m_objectRefTBO );
    glBindBuffer( GL_TEXTURE_BUFFER, m_objectRefTBO );

    int objectRefBufferSize = m_grid->GetObjectRefVector().size() * 4 * sizeof( float );
    glBufferData( GL_TEXTURE_BUFFER, objectRefBufferSize, 0, GL_DYNAMIC_DRAW );

    // Create object reference texture & bind it to the buffer
    glGenTextures( 1, &m_objectRefTex );
    glActiveTexture( GL_TEXTURE4 );
    glBindTexture( GL_TEXTURE_BUFFER, m_objectRefTex );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_objectRefTBO );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
}

// Buffers the grid into it's respective texture
void GLTracer::bufferGrid()
{
    glBindBuffer( GL_TEXTURE_BUFFER, m_accellStructureTBO );
    glm::vec4* structureBuffer = ( glm::vec4* ) glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY );

    const int* grid = m_grid->GetGridArray();

    for( int i = 0; i < m_grid->GetGridArrayLength(); ++i )
    {
        structureBuffer[ i ][ 0 ] = ( float )grid[ i ];
    }

    glUnmapBuffer( GL_TEXTURE_BUFFER );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    // Buffer the leaf nodes' object references
    glBindBuffer( GL_TEXTURE_BUFFER, m_objectRefTBO );
    glm::vec4* refBuffer = ( glm::vec4* ) glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY );

    const std::vector< int > objectRefVector = m_grid->GetObjectRefVector();

    for( int i = 0; i < objectRefVector.size(); ++i )
    {
        refBuffer[ i ][ 0 ] = ( float )objectRefVector[ i ];
    }

    glUnmapBuffer( GL_TEXTURE_BUFFER );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
}

// Performs initial setup of the kD tree texture and it's buffer
void GLTracer::generateKDTreeTex()
{
    // Generate tree texture
    glGenBuffers( 1, &m_accellStructureTBO );
    glBindBuffer( GL_TEXTURE_BUFFER, m_accellStructureTBO );

    int treeBufferSize = m_kdTree->GetTreeVector().size() * sizeof( glm::vec4 );
    glBufferData( GL_TEXTURE_BUFFER, treeBufferSize, 0, GL_DYNAMIC_DRAW );

    // Create accell structure texture & bind it to the buffer
    glGenTextures( 1, &m_accellStructureTex );
    glActiveTexture( GL_TEXTURE3 );
    glBindTexture( GL_TEXTURE_BUFFER, m_accellStructureTex );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_accellStructureTBO );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    // Generate object reference buffer
    glGenBuffers( 1, &m_objectRefTBO );
    glBindBuffer( GL_TEXTURE_BUFFER, m_objectRefTBO );

    int leafObjectBufferSize = m_kdTree->GetLeafObjectsVector().size() * sizeof( float );
    glBufferData( GL_TEXTURE_BUFFER, leafObjectBufferSize, 0, GL_DYNAMIC_DRAW );

    // Create object reference texture & bind it to the buffer
    glGenTextures( 1, &m_objectRefTex );
    glActiveTexture( GL_TEXTURE4 );
    glBindTexture( GL_TEXTURE_BUFFER, m_objectRefTex );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_objectRefTBO );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
}

// Buffers the kD Tree into it's respective texture
void GLTracer::bufferKDTree()
{
    // Buffer the tree itself
    glBindBuffer( GL_TEXTURE_BUFFER, m_accellStructureTBO );
    glm::vec4* treeBuffer = ( glm::vec4* ) glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY );

    const std::vector< glm::vec4 > treeVector = m_kdTree->GetTreeVector();

    for( int i = 0; i < treeVector.size(); ++i )
    {
        treeBuffer[ i ] = treeVector[ i ];
    }

    glUnmapBuffer( GL_TEXTURE_BUFFER );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    // Buffer the leaf nodes' object references
    glBindBuffer( GL_TEXTURE_BUFFER, m_objectRefTBO );
    float* leafObjectBuffer = ( float* ) glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY );

    const std::vector< int > leafObjectVector = m_kdTree->GetLeafObjectsVector();

    for( int i = 0; i < leafObjectVector.size(); ++i )
    {
        leafObjectBuffer[ i ] = ( float )leafObjectVector[ i ];
    }

    glUnmapBuffer( GL_TEXTURE_BUFFER );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
}

// Updates the OpenGL viewport size and dependant variables
void GLTracer::callbackResizeWindow( GLFWwindow* window, int width, int height )
{
    // Store new window size
    windowBounds.x = static_cast< float >( width );
    windowBounds.y = static_cast< float >( height );
    Controls::MouseOrigin = glm::ivec2( windowBounds.x / 2, windowBounds.y / 2 );

    // Calculate aspect ratio corrected viewport size and padding
    float letterboxRatio = glm::min( windowBounds.x / renderBounds.x, windowBounds.y / renderBounds.y );
    viewportBounds = renderBounds * letterboxRatio;
    viewportPadding = ( windowBounds - viewportBounds ) * 0.5f;

    // Inform the shader
    if( uniform_WindowSize != 0 && uniform_FOV != 0 )
    {
        glUseProgram( s_raytracerProgram );
        glUniform2f( uniform_WindowSize, viewportBounds.x, viewportBounds.y );
        glUniform1f( uniform_FOV, glm::radians( FOV ) );
    }
}

void GLTracer::callbackFocusWindow( GLFWwindow* window, int focused )
{
    if( focused == GL_FALSE )
    {
        Controls::SetMouseLock( false );
    }
}

// Updates the OpenGL viewport size and dependant variables
void GLTracer::callbackCloseWindow( GLFWwindow* window )
{
    // TODO: Figure out how to deallocate OpenGL objects from a static member function
    exit( 0 );
}
