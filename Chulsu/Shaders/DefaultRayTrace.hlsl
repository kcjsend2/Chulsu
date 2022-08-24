#define UINT_MAX 0xffffffff

RaytracingAccelerationStructure gRtScene : register(t0);

cbuffer FrameCB : register(b0)
{
    uint OutputTextureIndex : packoffset(c0.x);
    uint2 ScreenResolution : packoffset(c0.y);
    matrix gInvViewProj : packoffset(c1);
    float3 gCameraPos : packoffset(c5);
    float3 gShadowDirection : packoffset(c6);
}

cbuffer InstanceCB : register(b1)
{
    uint AlbedoTextureIndex : packoffset(c0.x);
    uint MetalicTextureIndex : packoffset(c0.y);
    uint RoughnessTextureIndex : packoffset(c0.z);
    uint NormalMapTextureIndex : packoffset(c0.w);

    uint VertexAttribIndex : packoffset(c1.x);
    uint IndexBufferIndex : packoffset(c1.y);
}

struct Vertex
{
    float3 position;
    float3 normal;
    float2 texCoord;
    float3 tangent;
    float3 biTangent;
};

//float3 GetWorldPosition(float2 texcoord, float depth)
//{
//    float4 clipSpaceLocation;
//    clipSpaceLocation.xy = texcoord * 2.0f - 1.0f;
//    clipSpaceLocation.y *= -1;
//    clipSpaceLocation.z = depth;
//    clipSpaceLocation.w = 1.0f;
//    float4 homogenousLocation = mul(clipSpaceLocation, gInvProj);
//    return homogenousLocation.xyz / homogenousLocation.w;
//}

float3 linearToSrgb(float3 c)
{
    float3 sq1 = sqrt(c);
    float3 sq2 = sqrt(sq1);
    float3 sq3 = sqrt(sq2);
    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
    return srgb;
}

inline RayDesc GenerateCameraRay(uint2 index, in float3 cameraPosition, in float4x4 invViewProj)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a world positon.
    float4 world = mul(float4(screenPos, 0, 1), invViewProj);
    world.xyz /= world.w;

    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = normalize(world.xyz - ray.Origin);

    return ray;
}

struct RayPayload
{
    float3 color;
};

[shader("raygeneration")]
void RayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float2 crd = float2(launchIndex.xy);
    float2 dims = float2(launchDim.xy);

    float2 d = ((crd / dims) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;
    
    RayDesc ray = GenerateCameraRay(launchIndex.xy, gCameraPos, gInvViewProj);

    ray.TMin = 0;
    ray.TMax = 100000;

    RayPayload payload;
    RWTexture2D<float4> output = ResourceDescriptorHeap[OutputTextureIndex];
    
    TraceRay(gRtScene, 0, 0xFFFFFFFF, 0, 0, 0, ray, payload);

    float3 col = linearToSrgb(payload.color);
    output[launchIndex.xy] = float4(col, 1);

}

struct ShadowPayload
{
    bool hit;
};

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = float3(0.4, 0.6, 0.2);
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    StructuredBuffer<Vertex> VertexBuffer = ResourceDescriptorHeap[VertexAttribIndex + GeometryIndex()];
    StructuredBuffer<uint> IndexBuffer = ResourceDescriptorHeap[IndexBufferIndex + GeometryIndex()];
    
    uint primIndex = PrimitiveIndex();
    
    uint i0 = IndexBuffer[primIndex * 3 + 0];
    uint i1 = IndexBuffer[primIndex * 3 + 1];
    uint i2 = IndexBuffer[primIndex * 3 + 2];
    
    Vertex v0 = VertexBuffer[i0];
    Vertex v1 = VertexBuffer[i1];
    Vertex v2 = VertexBuffer[i2];
    
    float hitT = RayTCurrent();
    float3 rayDirW = WorldRayDirection();
    float3 rayOriginW = WorldRayOrigin();
    
    // Find the world-space hit position
    float3 posW = rayOriginW + hitT * rayDirW;
    
    RayDesc ray;
    ray.Origin = posW;
    ray.Direction = gShadowDirection;
    ray.TMin = 0.01;
    ray.TMax = 100000;
    
    ShadowPayload shadowPayload;
    
    TraceRay(gRtScene, 0, 0xFFFFFFFF, 1, 0, 1, ray, shadowPayload);
    
    float factor = shadowPayload.hit ? 0.1 : 1.0;
    
    float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
    
    const float3 A = float3(1, 0, 0);
    const float3 B = float3(0, 1, 0);
    const float3 C = float3(0, 0, 1);

    payload.color = (A * barycentrics.x + B * barycentrics.y + C * barycentrics.z) * factor;
}

[shader("closesthit")]
void ShadowClosestHit(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.hit = true;
}

[shader("miss")]
void ShadowMiss(inout ShadowPayload payload)
{
    payload.hit = false;
}
