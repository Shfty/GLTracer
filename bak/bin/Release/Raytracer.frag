#version 140

const float nearPlane = 0.0;
const float farPlane = 10000.0;

in vec2 gl_FragCoord;
out vec4 fragColor;

uniform vec2 WindowSize;
uniform vec2 FOV;
uniform vec4 SkyColor;
uniform vec3 CameraPos;
uniform mat4 CameraRot;
uniform float AmbientIntensity;
uniform vec4 SkyLightColor;
uniform vec3 SkyLightDirection;
uniform int ObjectCount;
uniform int ObjectInfoSize;
uniform samplerBuffer ObjectInfoSampler;

/* Sphere Intersection
    ro - Ray Origin
    rd - Ray Direction
    rl - Ray Length
    rc - Ray Colour
    hp - Hit Position
    hn - Hit Normal
    sp - Sphere Position
    sr - Sphere Radius
    sc - Sphere Color
*/
bool isectSphere(
    in vec3 ro,
    in vec3 rd,
    inout float rl,
    inout vec4 rc,
    inout vec3 hp,
    inout vec3 hn,
    in vec3 sp,
    in float sr,
    in vec4 sc
    )
{
    bool hit = true;

    float rs = sr * sr;

    vec3 l = sp - ro;
    float tca = dot( l, rd );
    if( tca < 0 ) hit = false;

    float ds = dot( l, l ) - tca * tca;
    if( ds > sr * sr ) hit = false;

    float thc = sqrt( rs - ds );
    float t = tca - thc;
    if( t < nearPlane || t > rl ) hit = false;

    if( hit )
    {
        rl = t;
        rc = sc;
        hp = ro + ( rd * t );
        hn = normalize( hp - sp );
    }

    return hit;
}

/* Plane Intersection
    ro - Ray Origin
    rd - Ray Direction
    rl - Ray Length
    rc - Ray Colour
    hp - Hit Position
    hn - Hit Normal
    pp - Plane Position
    pr - Plane Rotation
    pc - Plane Color
*/
bool isectPlane(
    in vec3 ro,
    in vec3 rd,
    inout float rl,
    inout vec4 rc,
    inout vec3 hp,
    inout vec3 hn,
    in vec3 pp,
    in mat4 pr,
    in vec4 pc
    )
{
    bool hit = true;

    vec3 pn = normalize( vec3( pr * vec4( 0.0, 0.0, -1.0, 1.0 ) ) );

    float ndr = dot(pn, rd);
    if( ndr == 0.0 ) hit = false;

    float t = -(dot(pn, ro) - dot(pn, pp)) / dot(pn, rd);
    if( t < nearPlane || t > rl ) hit = false;

    if( hit )
    {
        rl = t;
        rc = pc;
        hp = ro + ( rd * t );
        hn = pn * -sign(ndr);
    }

    return hit;
}

/* Disc Intersection
    ro - Ray Origin
    rd - Ray Direction
    rl - Ray Length
    rc - Ray Colour
    hp - Hit Position
    hn - Hit Normal
    dp - Disc Position
    dr - Disc Rotation
    drad - Disc Radius
    dc - Disc Color
*/
bool isectDisc(
    in vec3 ro,
    in vec3 rd,
    inout float rl,
    inout vec4 rc,
    inout vec3 hp,
    inout vec3 hn,
    in vec3 dp,
    in mat4 dr,
    in float drad,
    in vec4 dc
    )
{
    bool hit = true;

    vec3 dn = normalize( vec3( dr * vec4( 0.0, 0.0, -1.0, 1.0 ) ) );

    // Plane intersection test
    float ndr = dot(dn, rd);
    if( ndr == 0.0 ) hit = false;

    float t = -(dot(dn, ro) - dot(dn, dp)) / dot(dn, rd);
    if( t < nearPlane || t > rl ) hit = false;

    // Disc intersection test
    vec3 pp = ro + (rd * t);
    vec3 ppl = pp - dp;
    float d = dot(ppl, ppl);
    if(d > drad * drad) hit = false;

    if( hit )
    {
        rl = t;
        rc = dc;
        hp = ro + ( rd * t );
        hn = dn * -sign(ndr);
    }

    return hit;
}

/* Cardinal Direction
    v - Vector
*/
vec3 cardinalDirection(vec3 v)
{
    int mi = 0;
    float mv = 0;

    for(int i = 0; i < 3; i++)
    {
        float avi = abs(v[i]);
        if(avi > mv)
        {
            mv = avi;
            mi = i;
        }
    }

    vec3 ov = vec3(0);
    ov[mi] = sign(v[mi]);

    return ov;
}

/* AABB Intersection
    ro - Ray Origin
    rd - Ray Direction
    rl - Ray Length
    rc - Ray Colour
    hp - Hit Position
    hn - Hit Normal
    b0 - Box Min
    b1 - Box Max
    bc - Box Color
*/
bool isectAABB(
    in vec3 ro,
    in vec3 rd,
    inout float rl,
    inout vec4 rc,
    inout vec3 hp,
    inout vec3 hn,
    in vec3 b0,
    in vec3 b1,
    in vec4 bc
    )
{
    bool hit = true;

    vec3 t;
    for( int i = 0 ; i < 3 ; i++ )
    {
        if( rd[i] >= 0 ) // CULL BACK FACE
          t[i] = ( b0[i] - ro[i] ) / rd[i];
        else
          t[i] = ( b1[i] - ro[i] ) / rd[i];
    }

    int mi = 0;
    float val = 0;

    for(int i = 0; i < 3; i++)
    {
        float iv = t[i];
        if(i == 0)
        {
            val = iv;
        }
        else
        {
            if(iv > val)
            {
                val = iv;
                mi = i;
            }
        }
    }

    if(t[mi] < nearPlane || t[mi] > rl) hit = false;

    vec3 pt = ro + ( rd * t[mi] ) ;
    int o1 = ( mi + 1 ) % 3 ; // i=0: o1=1, o2=2, i=1: o1=2,o2=0 etc.
    int o2 = ( mi + 2 ) % 3 ;

    if(pt[o1] < b0[o1] || pt[o1] > b1[o1]) hit = false;
    if(pt[o2] < b0[o2] || pt[o2] > b1[o2]) hit = false;

    if( hit )
    {
        rl = t;
        rc = bc;
        hp = ro + ( rd * t[mi] );
        vec3 sn = vec3(0);
        sn[mi] = sign(hp[mi] - ((b0[mi] + b1[mi]) * 0.5));
        hn = cardinalDirection(sn);
    }

    return hit;
}

mat4 rotationMatrix(vec3 axis, float angle)
{
    vec3 nAxis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat4(oc * nAxis.x * nAxis.x + c,           oc * nAxis.x * nAxis.y - nAxis.z * s,  oc * nAxis.z * nAxis.x + nAxis.y * s,  0.0,
                oc * nAxis.x * nAxis.y + nAxis.z * s,  oc * nAxis.y * nAxis.y + c,           oc * nAxis.y * nAxis.z - nAxis.x * s,  0.0,
                oc * nAxis.z * nAxis.x - nAxis.y * s,  oc * nAxis.y * nAxis.z + nAxis.x * s,  oc * nAxis.z * nAxis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

/* Convex Poly Intersection
    ro - Ray Origin
    rd - Ray Direction
    rl - Ray Length
    rc - Ray Colour
    hp - Hit Position
    hn - Hit Normal
    pp - Poly Position
    pr - Poly Rotation
    ps - Poly Scale
    pe - Poly Edges
    pc - Poly Color
*/
bool isectConvexPoly(
    in vec3 ro,
    in vec3 rd,
    inout float rl,
    inout vec4 rc,
    inout vec3 hp,
    inout vec3 hn,
    in vec3 pp,
    in mat4 pr,
    in float ps,
    in float pe,
    in vec4 pc
    )
{
    bool hit = true;

    vec3 pn = normalize( vec3( pr * vec4( 0.0, 0.0, -1.0, 1.0 ) ) );

    // Plane intersection test
    float ndr = dot(pn, rd);
    if( ndr == 0.0 ) hit = false;

    float t = -(dot(pn, ro) - dot(pn, pp)) / dot(pn, rd);
    if( t < nearPlane || t > rl ) hit = false;

    // Poly intersection test
    float edges = 4.0;//clamp( pe, 3.0, 10.0 );
    float scale = 1.0;

    vec3 pt = ro + ( rd * t ) ;
    vec3 bv = vec3( 0.0, -scale, 0.0 );
    bv = vec3( rotationMatrix( vec3( 0.0, 0.0, 1.0 ), 3.141 / edges ) * vec4( bv, 1.0 ) );
    vec3 wv = vec3( pr * vec4( bv, 1.0 ) ) + pp;

    for( int i = 0; i < int( edges ); i++ )
    {
        vec3 v0 = wv;

        bv = vec3( rotationMatrix( vec3( 0.0, 0.0, 1.0 ), 6.282 / edges ) * vec4( bv, 1.0 ) );
        wv = vec3( pr * vec4( bv, 1.0 ) ) + pp;
        vec3 v1 = wv;

        vec3 edge = normalize( v1 - v0 );

        vec3 c = normalize( pt - v0 );
        if( dot( pn, cross( edge, c ) ) < 0 ) hit = false;
    }

    if( hit )
    {
        rl = t;
        rc = pc;
        hp = ro + ( rd * t );
        hn = pn * -sign(ndr);
    }

    return hit;
}

// Extracts a 4x4 rotation matrix from a 1D buffer texture at offset o and returns it
mat4 extractMatrix( int o )
{
    float[16] matrixArray;
    matrixArray[0] = texelFetch( ObjectInfoSampler, o );
    matrixArray[1] = texelFetch( ObjectInfoSampler, o + 1 );
    matrixArray[2] = texelFetch( ObjectInfoSampler, o + 2 );
    matrixArray[3] = texelFetch( ObjectInfoSampler, o + 3 );
    matrixArray[4] = texelFetch( ObjectInfoSampler, o + 4 );
    matrixArray[5] = texelFetch( ObjectInfoSampler, o + 5 );
    matrixArray[6] = texelFetch( ObjectInfoSampler, o + 6 );
    matrixArray[7] = texelFetch( ObjectInfoSampler, o + 7 );
    matrixArray[8] = texelFetch( ObjectInfoSampler, o + 8 );
    matrixArray[9] = texelFetch( ObjectInfoSampler, o + 9 );
    matrixArray[10] = texelFetch( ObjectInfoSampler, o + 10 );
    matrixArray[11] = texelFetch( ObjectInfoSampler, o + 11 );
    matrixArray[12] = texelFetch( ObjectInfoSampler, o + 12 );
    matrixArray[13] = texelFetch( ObjectInfoSampler, o + 13 );
    matrixArray[14] = texelFetch( ObjectInfoSampler, o + 14 );
    matrixArray[15] = texelFetch( ObjectInfoSampler, o + 15 );

    return mat4( matrixArray[0], matrixArray[4], matrixArray[8], matrixArray[12],
                 matrixArray[1], matrixArray[5], matrixArray[9], matrixArray[13],
                 matrixArray[2], matrixArray[6], matrixArray[10], matrixArray[14],
                 matrixArray[3], matrixArray[7], matrixArray[11], matrixArray[15]
               );
}

void main()
{
    // Calculate relative screen-space position
    vec2 pointCoord = vec2(0);
    for(int i = 0; i < 2; i++)
    {
        pointCoord[i] = gl_FragCoord[i] / WindowSize[i];
    }

    // Ray Direction
    float xMag = ((pointCoord.x - 0.5) * 2) * tan(FOV.x);
    float yMag = ((pointCoord.y - 0.5) * 2) * tan(FOV.y);
    vec3 rd = normalize(vec3( xMag, yMag, -1.0 ));
    rd = vec3(CameraRot * vec4(rd, 1.0));

    // Ray Hit Data
    vec3 hp = vec3(0); // Position
    vec3 hn = vec3(0); // Normal

    float rl;
    vec4 outColor = SkyColor;

    // Primary ray intersection
    rl = farPlane;

    for( int i = 0; i < ObjectCount; i++ )
    {
        // Prefetch object parameters
        // Object Type
        float type = texelFetch( ObjectInfoSampler, i * ObjectInfoSize );

        // Position
        vec3 pos = vec3( 0 );
        pos.x = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 1 );
        pos.y = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 2 );
        pos.z = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 3 );

        // Scale
        float scale = 1.0;
        scale = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 4 );

        // Max
        vec3 maxPos = vec3( 0 );
        maxPos.x = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 5 );
        maxPos.y = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 6 );
        maxPos.z = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 7 );

        // Sides
        float sides = 3.0;
        sides = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 8 );

        // Color
        vec4 color = vec4( 0 );
        color.r = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 9 );
        color.g = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 10 );
        color.b = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 11 );
        color.a = texelFetch( ObjectInfoSampler, (i * ObjectInfoSize) + 12 );

        // Portal

        // Diffuse

        // Specular

        // Refractive Index

        // Rotation Matrix
        mat4 rot = mat4(1);
        rot = extractMatrix( (i * ObjectInfoSize) + 13 );

        // Portal Matrix

        // Choose object type and apply relevant parameters
        switch( int( type ) )
        {
            case 0:
                isectSphere( CameraPos, rd, rl, outColor, hp, hn, pos, scale, color );
                break;
            case 1:
                isectPlane( CameraPos, rd, rl, outColor, hp, hn, pos, rot, color );
                break;
            case 2:
                isectDisc( CameraPos, rd, rl, outColor, hp, hn, pos, rot, scale, color );
                break;
            case 3:
                isectAABB( CameraPos, rd, rl, outColor, hp, hn, pos, maxPos, color );
                break;
            case 4:
                isectConvexPoly( CameraPos, rd, rl, outColor, hp, hn, pos, rot, scale, sides, color );
                break;
            default:
                break;
        }
    }

    // Celestial bodies
    isectSphere( CameraPos, rd, rl, outColor, hp, hn, SkyLightDirection * 1000, 100.0, vec4( SkyLightColor.xyz, -1.0 ) );
    isectSphere( CameraPos, rd, rl, outColor, hp, hn, -SkyLightDirection * 1000, 100.0, vec4( 1.0, 1.0, 1.0, -1.0 ) );

    // Shadow ray intersection
    vec3 ro = hp + (hn * 0.0001);
    rd = normalize(SkyLightDirection);
    rl = farPlane;
    bool occluded = false;
    vec3 dv3; // Dummies
    vec4 dv4;

/*
    // Playground objects
    if( isectPlane( ro, rd, rl, dv4, dv3, dv3, vec3( 0, -5, 0 ), vec3( 0, -1, 0 ), vec4( 0 ) ) ) occluded = true;
    //if( isectSphere( ro, rd, rl, dv4, dv3, dv3, spherePos, 5.0, vec4(0) ) ) occluded = true;
    if( isectDisc( ro, rd, rl, dv4, dv3, dv3, vec3( 30, 0, -70 ), vec3( 0, 1, 1 ), 5.0, vec4( 0 ) ) ) occluded = true;
    if( isectAABB( ro, rd, rl, dv4, dv3, dv3, vec3( -50, -15, -50 ), vec3( -35, 5, -35 ), vec4( 0 ) ) ) occluded = true;

    // Walls
    if( isectConvexPoly( ro, rd, rl, dv4, dv3, dv3, vec3( 0, -50, -80 ), vec3( 0, 0, 1 ), 100.0, 4, vec4( 0 ) ) ) occluded = true;
    if( isectConvexPoly( ro, rd, rl, dv4, dv3, dv3, vec3( 0, -50, 40 ), vec3( 0, 0, -1 ), 100.0, 4, vec4( 0 ) ) ) occluded = true;
    if( isectConvexPoly( ro, rd, rl, dv4, dv3, dv3, vec3( 60, -50, -20 ), vec3( -1, 0, 0 ), 100.0, 4, vec4( 0 ) ) ) occluded = true;
    if( isectConvexPoly( ro, rd, rl, dv4, dv3, dv3, vec3( -60, -50, -20 ), vec3( 1, 0, 0 ), 100.0, 4, vec4( 0 ) ) ) occluded = true;
*/
    // Alpha of -1.0 == fullbright
    if( outColor != SkyColor && outColor.a != -1.0 )
    {
        float brightness = 0.0;
        if( !occluded )
        {
            float df = max(0.0, dot(hn, normalize(SkyLightDirection)));

            float sf = 0.0;
/*          Specular code - not currently working properly
            if(df > 0.0)
            {
                vec3 ito = normalize( CameraPos - hp );
                vec3 rl = normalize( reflect( -SkyLightDirection, hn ) );
                sf = dot( ito, rl );
                sf = pow( sf, 2 );
            }
*/

            brightness = sf + df + AmbientIntensity;
        }
        else
        {
            brightness = AmbientIntensity;
        }

        outColor *= SkyLightColor;
        outColor *= brightness;
    }

    for(int i = 0; i < 4; i++)
    {
        outColor[i] = min(outColor[i], 1.0);
    }
    outColor[3] = 1.0;

    fragColor = outColor;
}
