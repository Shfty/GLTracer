#include "GLTracer.h"
#include "GLError.h"

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
glm::vec2 windowBounds = glm::vec2( 2560.0f, 1440.0f );
static GLint uniform_WindowSize = -1;
static GLint uniform_FOV = -1;

static GLuint s_raytracerProgram = 0;

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

    callbackResizeWindow( 0, windowBounds.x, windowBounds.y );
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
        GL(glDeleteProgram( m_raytracerProgram ));
    }

    if( m_basicProgram != 0 )
    {
        GL(glDeleteProgram( m_basicProgram ));
    }

    glfwDestroyWindow( m_window );

    GL(glDeleteBuffers( 1, &m_vertexBuffer ));
    GL(glDeleteBuffers( 1, &m_texCoordBuffer ));

    GL(glDeleteTextures( 1, &m_objectInfoTex ));
    GL(glDeleteBuffers( 1, &m_objectInfoTBO ));

    GL(glDeleteTextures( 1, &m_accellStructureTex ));
    GL(glDeleteBuffers( 1, &m_accellStructureTBO ));

    GL(glDeleteTextures( 1, &m_objectRefTex ));
    GL(glDeleteBuffers( 1, &m_objectRefTBO ));

    GL(glDeleteTextures( 1, &m_screenColorTexture ));
    GL(glDeleteTextures( 1, &m_screenDepthTexture ));
    GL(glDeleteFramebuffers( 1, &m_framebuffer ));

    glfwTerminate();
}

// Performs per-frame changes to the world state
void GLTracer::Update()
{
    WorldClock::Instance()->Update();

    // Raytracer program needs to be active to receive data
    glUseProgram( m_raytracerProgram );

    // Camera
    m_camera->Update( scene->GetObjects() );

    // Sky light direction
    float deltaSkyLightAngle = SKYLIGHT_ROTATE_PER_SEC * WorldClock::Instance()->DeltaTime();
    skyLightDirection = glm::normalize( glm::vec3( glm::rotate(glm::mat4(1.0),  deltaSkyLightAngle, glm::vec3( 0, 0, 1 ) ) * glm::vec4( skyLightDirection, 1.0f ) ) );

    glm::vec4 skyColor = glm::mix( dayColor, nightColor, ( -skyLightDirection.y + 1 ) / 2 );
    scene->Update( skyColor );
#if ACCELL_STRUCTURE == ACC_GRID
    m_grid->Update( scene->GetObjects(), m_camera->GetPosition(), glm::vec3( m_camera->GetRotation() * glm::vec4( 0, 0, -1, 0 ) ) );
#endif

    // Window title info readout
    static float acc = 0;
    static int frames = 0;
    acc += WorldClock::Instance()->DeltaTime();
    frames++;

    if( acc > 1.0f )
    {
        std::stringstream ss;
        ss << windowTitle << std::string( " | FPS: " ) << frames;
        ss << " | Internal Resolution: " << windowBounds.x << "x" << windowBounds.y;
        ss << " | Window Resolution: " << windowBounds.x << "x" << windowBounds.y;
        glfwSetWindowTitle( m_window, ss.str().c_str() );
        std::cout << "FPS: " << frames << std::endl;
        acc = 0.0;
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
    setupRenderTexture();

    // Update world objects and prepare kD tree
    bufferPrimitives( scene->GetUpdatedObjects() );
#if ACCELL_STRUCTURE == ACC_GRID
    bufferGrid();
#endif

#if ACCELL_STRUCTURE == ACC_KDTREE
    m_kdTree->BuildTree( scene->GetObjects() );
    walkKDTree();
    bufferKDTree();
#endif

    // Update uniforms
    //Camera
    glm::vec3 cameraPos = m_camera->GetPosition();
    GL(glUniform3f( m_uniform_CameraPos, cameraPos.x, cameraPos.y, cameraPos.z ));
    GL(glUniformMatrix4fv( m_uniform_CameraRot, 1, GL_FALSE, glm::value_ptr( m_camera->GetRotation() ) ));
    GL(glUniformMatrix4fv( m_uniform_CameraRotInverse, 1, GL_FALSE, glm::value_ptr( glm::inverse( m_camera->GetRotation() ) ) ));

    //Sky
    GL(glUniform3f( m_uniform_SkyLightDirection, skyLightDirection.x, skyLightDirection.y, skyLightDirection.z ));

    // Render to internal texture
    GL(glBindFramebuffer( GL_FRAMEBUFFER, m_framebuffer ));
    GL(glViewport( 0, 0, windowBounds.x, windowBounds.y ));
    GL(glUseProgram( m_raytracerProgram ));

    GL(glEnable( GL_DEPTH_TEST ));
    GL(glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ));
    GL(glDrawArrays( GL_QUADS, 0, 4 ));

    // Draw debug components
    GL(glUseProgram( 0 ));

    GL(glMatrixMode( GL_PROJECTION ));
    GL(glLoadIdentity());
    GL(glMultMatrixf( glm::value_ptr( m_projectionMatrix ) ));

    GL(glMatrixMode( GL_MODELVIEW ));
    GL(glLoadIdentity());
    GL(glMultMatrixf( glm::value_ptr( glm::inverse( m_camera->GetRotation() ) ) ));
    GL(glTranslatef( -m_camera->GetPosition().x, -m_camera->GetPosition().y, -m_camera->GetPosition().z ));

    glBegin( GL_TRIANGLES );
        glVertex3f( 0.0, 20.0, -10.0 );
        glVertex3f( 10.0, 0.0, -10.0 );
        glVertex3f(-10.0, 0.0, -10.0 );
    glEnd();
    handle_error();

#if ACCELL_STRUCTURE == ACC_GRID
#ifdef RENDER_DEBUG
    m_grid->Draw();
#endif
#endif

    GL(glDisable( GL_DEPTH_TEST ));

    // Draw to screen
    GL(glBindFramebuffer( GL_FRAMEBUFFER, 0 ));
    int fb_width = 0;
    int fb_height = 0;
    GL(glfwGetFramebufferSize( m_window, &fb_width, &fb_height ));
    GL(glViewport( 0, 0, fb_width, fb_height ));
    GL(glUseProgram( m_basicProgram ));

    GL(glClear( GL_COLOR_BUFFER_BIT ));
    GL(glDrawArrays( GL_QUADS, 0, 4 ));

#ifdef RENDER_CROSSHAIR
    GL(glUseProgram( 0 ));
    GL(glLoadIdentity());
    GL(glColor3f( 1, 1, 1 ));
    GL(glLineWidth( 1.0f ));
    glBegin( GL_LINES );
        glVertex3f(-0.025f, 0.0f,-1.0f );
        glVertex3f( 0.025f, 0.0f,-1.0f );

        glVertex3f( 0.0f,-0.025f,-1.0f );
        glVertex3f( 0.0f, 0.025f,-1.0f );
    glEnd();
    handle_error();
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
    GL(glBindBuffer( GL_TEXTURE_BUFFER, m_objectInfoTBO ));
    glm::vec4* p = ( glm::vec4* ) glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY );

    // Shared parameters
    // Info Cell
    p[ ( idx * INFO_PACKET_SIZE ) + 0 ] = glm::vec4( Primitive->ID, // ID
                                                  ( GLfloat )Primitive->Type,
                                                  -1.0f,
                                                  -1.0f );

    // World Matrix
    glm::mat4 transMatrix = glm::translate( glm::mat4(1.0), Primitive->Position );
    glm::mat4 rotMatrix = glm::mat4_cast( Primitive->Orientation );
    glm::mat4 scaleMatrix = glm::scale( glm::mat4(1.0), Primitive->Scale );


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

    GL(glUnmapBuffer( GL_TEXTURE_BUFFER ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, 0 ));
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
    GL(glClearColor( 1.0f, 0.0f, 0.0f, 1.0f ));
}

// Generate and setup framebuffer, render texture
void GLTracer::setupRenderTexture()
{
    if (m_screenColorTexture != 0) {
        GL(glDeleteTextures(1, &m_screenColorTexture));
    }

    if (m_screenDepthTexture != 0) {
        GL(glDeleteTextures(1, &m_screenDepthTexture));
    }

    if (m_framebuffer != 0) {
        GL(glDeleteFramebuffers(1, &m_framebuffer));
    }

    // Color Texture
    GL(glActiveTexture( GL_TEXTURE0 ));
    GL(glGenTextures( 1, &m_screenColorTexture ));
    GL(glBindTexture( GL_TEXTURE_2D, m_screenColorTexture ));
    GL(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP ));
    GL(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP ));
    GL(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ));
    GL(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ));
    GL(glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, windowBounds.x, windowBounds.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL ));

    // Depth Texture
    GL(glActiveTexture( GL_TEXTURE1 ));
    GL(glGenTextures( 1, &m_screenDepthTexture ));
    GL(glBindTexture( GL_TEXTURE_2D, m_screenDepthTexture ));
    GL(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP ));
    GL(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP ));
    GL(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ));
    GL(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ));
    GL(glTexParameteri( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY ));
    GL(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE ));
    GL(glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL ));
    GL(glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, windowBounds.x, windowBounds.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL ));

    // Generate framebuffer and bind color texture
    GL(glGenFramebuffers( 1, &m_framebuffer ));
    GL(glBindFramebuffer( GL_FRAMEBUFFER, m_framebuffer ));
    GL(glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_screenColorTexture, 0 ));
    GL(glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_screenDepthTexture, 0 ));

    GL(GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER ));

    if( status != GL_FRAMEBUFFER_COMPLETE )
    {
        std::cerr << "Unable to complete Framebuffer setup, quitting...";
        exit( -1 );
    }
}

// Generate the screen quad vertex buffer and load in vertices
void GLTracer::setupVertexBuffer()
{
    GL(glGenBuffers( 1, &m_vertexBuffer ));
    GL(glBindBuffer( GL_ARRAY_BUFFER, m_vertexBuffer ));

    // Vertices - Location 0
    glm::vec3 quadVertices[4] = {
        glm::vec3(-1.0, 1.0, 0.0 ),
        glm::vec3( 1.0, 1.0, 0.0 ),
        glm::vec3( 1.0,-1.0, 0.0 ),
        glm::vec3(-1.0,-1.0, 0.0 )
    };

    GL(glBufferData( GL_ARRAY_BUFFER, 4 * sizeof( glm::vec3 ), quadVertices, GL_STATIC_DRAW ));

    GL(glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 ));
    GL(glEnableVertexAttribArray( 0 ));

    // Texture Coordinates - Location 1
    GL(glGenBuffers( 1, &m_texCoordBuffer ));
    GL(glBindBuffer( GL_ARRAY_BUFFER, m_texCoordBuffer ));

    glm::vec2 quadTexCoords[4] = {
        glm::vec2( 0.0, 1.0 ),
        glm::vec2( 1.0, 1.0 ),
        glm::vec2( 1.0, 0.0 ),
        glm::vec2( 0.0, 0.0 )
    };

    GL(glBufferData( GL_ARRAY_BUFFER, 4 * sizeof( glm::vec2 ), quadTexCoords, GL_STATIC_DRAW ));

    GL(glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, 0 ));
    GL(glEnableVertexAttribArray( 1 ));
}

// Attempt to compile and link shaders,
// bind the correct vertex/fragment variable names,
// send the relevant info logs to standard output
void GLTracer::compileShaders()
{
    std::cout << "Compile Shaders" << std::endl;

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
        std::cout << "Deleting " << m_basicProgram << std::endl;
        GL(glDeleteProgram( m_basicProgram ));
    }

    GL(m_basicProgram = glCreateProgram());
    std::cout << "Program " << m_basicProgram << std::endl;

    GL(glBindAttribLocation(m_basicProgram, 0, "vertex"));
    GL(glBindFragDataLocation(m_basicProgram, 0, "fragColor"));
    GL(glAttachShader( m_basicProgram, m_basicVS->GetID() ));
    GL(glAttachShader( m_basicProgram, m_basicFS->GetID() ));
    GL(glLinkProgram( m_basicProgram ));

    GLint basicLinked = GL_FALSE;
    GL(glGetProgramiv( m_basicProgram, GL_LINK_STATUS, &basicLinked ));
    std::cout << "Link status: " << (basicLinked ? "GL_TRUE" : "GL_FALSE") << std::endl;

    GLchar basicInfoLog[ GL_INFO_LOG_LENGTH ] = { 0 };
    GL(glGetProgramInfoLog( m_basicProgram, GL_INFO_LOG_LENGTH, NULL, basicInfoLog ));
    std::cout << "Info log:" << std::endl << basicInfoLog << std::endl;

    // Raytracer Program
    if( m_raytracerProgram != 0 )
    {
        GL(glDeleteProgram( m_raytracerProgram ));
    }

    GL(m_raytracerProgram = glCreateProgram());

    s_raytracerProgram = m_raytracerProgram;
    GL(glAttachShader( m_raytracerProgram, m_basicVS->GetID() ));
    GL(glAttachShader( m_raytracerProgram, m_raytracerFS->GetID() ));
    GL(glLinkProgram( m_raytracerProgram ));

    GLint raytracerLinked = GL_FALSE;
    GL(glGetProgramiv( m_raytracerProgram, GL_LINK_STATUS, &raytracerLinked ));
    std::cout << "Program " << m_raytracerProgram << " link status: " << (raytracerLinked ? "GL_TRUE" : "GL_FALSE") << std::endl;

    GLchar raytracerInfoLog[ GL_INFO_LOG_LENGTH ] = { 0 };
    GL(glGetProgramInfoLog( m_raytracerProgram, GL_INFO_LOG_LENGTH, NULL, raytracerInfoLog ));
    std::cout << std::endl << "Raytracer Program info log:" << std::endl << raytracerInfoLog << std::endl;

    setupUniforms();
}

void GLTracer::setupUniforms()
{
    // Retrieve Basic uniform IDs and set screen texture location
    GL(glUseProgram( m_basicProgram ));
    GL(glUniform1i( glGetUniformLocation( m_basicProgram, "ScreenTextureSampler" ), 0 ));

    // Retrieve Raytracer uniform IDs and set uniforms
    GL(glUseProgram( m_raytracerProgram ));

    GL(uniform_WindowSize = glGetUniformLocation( m_raytracerProgram, "WindowSize" ));
    GL(uniform_FOV = glGetUniformLocation( m_raytracerProgram, "FOV" ));
    GL(glUniform2f( uniform_WindowSize, windowBounds.x, windowBounds.y ));
    GL(glUniform1f( uniform_FOV, glm::radians( FOV ) ));

    GL(m_uniform_CameraPos = glGetUniformLocation( m_raytracerProgram, "CameraPos" ));
    GL(m_uniform_CameraRot = glGetUniformLocation( m_raytracerProgram, "CameraRot" ));
    GL(m_uniform_CameraRotInverse = glGetUniformLocation( m_raytracerProgram, "CameraRotInverse" ));

    GL(m_uniform_AmbientIntensity = glGetUniformLocation( m_raytracerProgram, "AmbientIntensity" ));
    GL(m_uniform_SkyLightColor = glGetUniformLocation( m_raytracerProgram, "SkyLightColor" ));
    GL(m_uniform_SkyLightDirection = glGetUniformLocation( m_raytracerProgram, "SkyLightDirection" ));

    GL(glUniform1f( m_uniform_AmbientIntensity, AMBIENT_INTENSITY ));
    GL(glUniform4f( m_uniform_SkyLightColor, 1.0, 1.0, 1.0, 1.0 ));

    GL(glUniform1i( glGetUniformLocation( m_raytracerProgram, "GridSubdivisions" ), m_grid->GetSubdivisions() ));
    glm::vec3 p0 = m_grid->GetMinBound();
    GL(glUniform3f( glGetUniformLocation( m_raytracerProgram, "GridMinBound" ), p0.x, p0.y, p0.z ));
    glm::vec3 p1 = m_grid->GetMaxBound();
    GL(glUniform3f( glGetUniformLocation( m_raytracerProgram, "GridMaxBound" ), p1.x, p1.y, p1.z ));
    glm::vec3 cs = m_grid->GetCellSize();
    GL(glUniform3f( glGetUniformLocation( m_raytracerProgram, "GridCellSize" ), cs.x, cs.y, cs.z ));

    GL(glUniform1i( glGetUniformLocation( m_raytracerProgram, "PrimitiveSampler" ), 2 ));
    GL(glUniform1i( glGetUniformLocation( m_raytracerProgram, "AccellStructureSampler" ), 3 ));
    GL(glUniform1i( glGetUniformLocation( m_raytracerProgram, "ObjectRefSampler" ), 4 ));
}

// Performs initial setup of the object info texture and it's buffer
void GLTracer::generateObjectInfoTex()
{
    GL(glGenBuffers( 1, &m_objectInfoTBO ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, m_objectInfoTBO ));

    int bufferSize = scene->GetObjects().size() * INFO_PACKET_SIZE * sizeof( glm::vec4 );
    GL(glBufferData( GL_TEXTURE_BUFFER, bufferSize, 0, GL_DYNAMIC_DRAW ));

    // Create object info texture & bind it to the buffer
    GL(glGenTextures( 1, &m_objectInfoTex ));
    GL(glActiveTexture( GL_TEXTURE2 ));
    GL(glBindTexture( GL_TEXTURE_BUFFER, m_objectInfoTex ));
    GL(glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_objectInfoTBO ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, 0 ));
}

// Buffers the provided set of world objects to the info texture
void GLTracer::bufferPrimitives( std::vector< Primitive* > primitives )
{
    // Load world objects into info texture
    for( uint16_t i = 0; i < primitives.size(); i++ )
    {
        BufferPrimitive( primitives[ i ], i );
    }

    GL(glUseProgram( m_raytracerProgram ));
    GL(glUniform1i( glGetUniformLocation( m_raytracerProgram, "ObjectCount" ), scene->GetObjects().size() ));
    GL(glUniform1i( glGetUniformLocation( m_raytracerProgram, "ObjectInfoSize" ), INFO_PACKET_SIZE ));
}

// Performs initial setup of the grid texture and it's buffer
void GLTracer::generateGridTex()
{
    // Generate tree texture
    GL(glGenBuffers( 1, &m_accellStructureTBO ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, m_accellStructureTBO ));

    int gridBufferSize = m_grid->GetGridArrayLength() * 4 * sizeof( float );
    GL(glBufferData( GL_TEXTURE_BUFFER, gridBufferSize, 0, GL_DYNAMIC_DRAW ));

    // Create accell structure texture & bind it to the buffer
    GL(glGenTextures( 1, &m_accellStructureTex ));
    GL(glActiveTexture( GL_TEXTURE3 ));
    GL(glBindTexture( GL_TEXTURE_BUFFER, m_accellStructureTex ));
    GL(glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_accellStructureTBO ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, 0 ));

    // Generate object reference buffer
    GL(glGenBuffers( 1, &m_objectRefTBO ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, m_objectRefTBO ));

    int objectRefBufferSize = m_grid->GetObjectRefVector().size() * 4 * sizeof( float );
    GL(glBufferData( GL_TEXTURE_BUFFER, objectRefBufferSize, 0, GL_DYNAMIC_DRAW ));

    // Create object reference texture & bind it to the buffer
    GL(glGenTextures( 1, &m_objectRefTex ));
    GL(glActiveTexture( GL_TEXTURE4 ));
    GL(glBindTexture( GL_TEXTURE_BUFFER, m_objectRefTex ));
    GL(glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_objectRefTBO ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, 0 ));
}

// Buffers the grid into it's respective texture
void GLTracer::bufferGrid()
{
    GL(glBindBuffer( GL_TEXTURE_BUFFER, m_accellStructureTBO ));
    GL(glm::vec4* structureBuffer = ( glm::vec4* ) glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY ));

    const int* grid = m_grid->GetGridArray();

    for( int i = 0; i < m_grid->GetGridArrayLength(); ++i )
    {
        structureBuffer[ i ][ 0 ] = ( float )grid[ i ];
    }

    GL(glUnmapBuffer( GL_TEXTURE_BUFFER ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, 0 ));

    // Buffer the leaf nodes' object references
    GL(glBindBuffer( GL_TEXTURE_BUFFER, m_objectRefTBO ));
    GL(glm::vec4* refBuffer = ( glm::vec4* ) glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY ));

    const std::vector< int > objectRefVector = m_grid->GetObjectRefVector();

    for( int i = 0; i < objectRefVector.size(); ++i )
    {
        refBuffer[ i ][ 0 ] = ( float )objectRefVector[ i ];
    }

    GL(glUnmapBuffer( GL_TEXTURE_BUFFER ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, 0 ));
}

// Performs initial setup of the kD tree texture and it's buffer
void GLTracer::generateKDTreeTex()
{
    // Generate tree texture
    GL(glGenBuffers( 1, &m_accellStructureTBO ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, m_accellStructureTBO ));

    int treeBufferSize = m_kdTree->GetTreeVector().size() * sizeof( glm::vec4 );
    GL(glBufferData( GL_TEXTURE_BUFFER, treeBufferSize, 0, GL_DYNAMIC_DRAW ));

    // Create accell structure texture & bind it to the buffer
    GL(glGenTextures( 1, &m_accellStructureTex ));
    GL(glActiveTexture( GL_TEXTURE3 ));
    GL(glBindTexture( GL_TEXTURE_BUFFER, m_accellStructureTex ));
    GL(glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_accellStructureTBO ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, 0 ));

    // Generate object reference buffer
    GL(glGenBuffers( 1, &m_objectRefTBO ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, m_objectRefTBO ));

    int leafObjectBufferSize = m_kdTree->GetLeafObjectsVector().size() * sizeof( float );
    GL(glBufferData( GL_TEXTURE_BUFFER, leafObjectBufferSize, 0, GL_DYNAMIC_DRAW ));

    // Create object reference texture & bind it to the buffer
    GL(glGenTextures( 1, &m_objectRefTex ));
    GL(glActiveTexture( GL_TEXTURE4 ));
    GL(glBindTexture( GL_TEXTURE_BUFFER, m_objectRefTex ));
    GL(glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_objectRefTBO ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, 0 ));
}

// Buffers the kD Tree into it's respective texture
void GLTracer::bufferKDTree()
{
    // Buffer the tree itself
    GL(glBindBuffer( GL_TEXTURE_BUFFER, m_accellStructureTBO ));
    GL(glm::vec4* treeBuffer = ( glm::vec4* ) glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY ));

    const std::vector< glm::vec4 > treeVector = m_kdTree->GetTreeVector();

    for( int i = 0; i < treeVector.size(); ++i )
    {
        treeBuffer[ i ] = treeVector[ i ];
    }

    GL(glUnmapBuffer( GL_TEXTURE_BUFFER ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, 0 ));

    // Buffer the leaf nodes' object references
    GL(glBindBuffer( GL_TEXTURE_BUFFER, m_objectRefTBO ));
    float* leafObjectBuffer = ( float* ) glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY );

    const std::vector< int > leafObjectVector = m_kdTree->GetLeafObjectsVector();

    for( int i = 0; i < leafObjectVector.size(); ++i )
    {
        leafObjectBuffer[ i ] = ( float )leafObjectVector[ i ];
    }

    GL(glUnmapBuffer( GL_TEXTURE_BUFFER ));
    GL(glBindBuffer( GL_TEXTURE_BUFFER, 0 ));
}

// Updates the OpenGL viewport size and dependent variables
void GLTracer::callbackResizeWindow( GLFWwindow* window, int width, int height )
{
    std::cout << "Window Resized: " << width << ", " << height << std::endl;

    // Store new window size
    windowBounds.x = static_cast< float >( width );
    windowBounds.y = static_cast< float >( height );
    Controls::MouseOrigin = glm::ivec2( windowBounds.x / 2, windowBounds.y / 2 );

    // Inform the shader
    GL(glUseProgram( s_raytracerProgram ));
    if( uniform_WindowSize != -1 )
    {
        GL(glUniform2f( uniform_WindowSize, windowBounds.x, windowBounds.y ));
    }

    if( uniform_FOV != -1 )
    {
        GL(glUniform1f( uniform_FOV, glm::radians( FOV ) ));
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
