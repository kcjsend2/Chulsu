#define UINT_MAX 0xffffffff

struct VertexAttribute
{
    float3 position;
    float3 normal;
    float2 texCoord;
    float3 tangent;
    float3 biTangent;
};

cbuffer RayTraceCB : register(b0)
{
    uint AlbedoTextureIndex : packoffset(c0.x);
    uint MetalicTextureIndex : packoffset(c0.y);
    uint RoughnessTextureIndex : packoffset(c0.z);
    uint NormalMapTextureIndex : packoffset(c0.w);

    uint VertexAttribIndex : packoffset(c1.x);
    uint IndexBufferIndex : packoffset(c1.y);
}

float3 linearToSrgb(float3 c)
{
    // Based on http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
    float3 sq1 = sqrt(c);
    float3 sq2 = sqrt(sq1);
    float3 sq3 = sqrt(sq2);
    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
    return srgb;
}

struct RayPayload
{
    float3 color;
};

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float2 crd = float2(launchIndex.xy);
    float2 dims = float2(launchDim.xy);

    float2 d = ((crd / dims) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;
    
    RayDesc ray;
    ray.Origin = float3(0, 0, -2);
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));

    ray.TMin = 0;
    ray.TMax = 100000;

    RayPayload payload;
    RaytracingAccelerationStructure rtScene = ResourceDescriptorHeap[0];
    RWTexture2D<float4> output = ResourceDescriptorHeap[1];
    
    TraceRay(rtScene, 0, 0xFFFFFFFF, 0, 0, 0, ray, payload);

    float3 col = linearToSrgb(payload.color);
    output[launchIndex.xy] = float4(col, 1);

}

[shader("miss")]
void miss(inout RayPayload payload)
{
    payload.color = float3(0.4, 0.6, 0.2);
}

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
    
    const float3 A = float3(1, 0, 0);
    const float3 B = float3(0, 1, 0);
    const float3 C = float3(0, 0, 1);

    payload.color = A * barycentrics.x + B * barycentrics.y + C * barycentrics.z;
}
