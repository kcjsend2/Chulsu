#define PI 3.1415926535f
#define UINT_MAX 0xffffffff

#define DIRECTIONAL_LIGHT 0
#define SPOT_LIGHT 1
#define POINT_LIGHT 2

struct Light
{
    float3 position;
    int active;

    float3 direction;
    float range;

    float3 color;
    uint type;

    float outerCosine;
    float innerCosine;
    int castShadows;
};

struct GeometryInfo
{
    uint VertexOffset;
    uint IndexOffset;
    
    uint AlbedoTextureIndex;
    uint MetalicTextureIndex;
    uint RoughnessTextureIndex;
    uint NormalMapTextureIndex;
    uint OpacityMapTextureIndex;
};

struct Vertex
{
    float3 position;
    float3 normal;
    float2 texCoord;
    float3 tangent;
    float3 biTangent;
};

SamplerState gAnisotropicWrap : register(s0);
RaytracingAccelerationStructure gRtScene : register(t0);

float SchlickFresnel(float3 F0, float3 H, float3 L)
{
    return F0 + (1.0f - F0) * pow(1.0f - saturate(dot(H, L)), 5);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 DirectionalLightPBR(Light light, float3 N, float3 V, float3 albedo, float metallic, float roughness)
{
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);
    
     // calculate per-light radiance
    float3 L = normalize(-light.direction.xyz);
    float3 H = normalize(V + L);
    
    float3 radiance = light.color.xyz;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = SchlickFresnel(F0, L, H);
           
    float3 nominator = NDF * G * F;
    float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    float3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0
        
        // kS is equal to Fresnel
    float3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
    kD *= 1.0 - metallic;

        // scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
    float3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    return Lo;
}

cbuffer FrameCB : register(b0)
{
    uint OutputTextureIndex : packoffset(c0.x);
    uint2 ScreenResolution : packoffset(c0.y);
    uint gLightIndex : packoffset(c0.w);
    matrix gInvViewProj : packoffset(c1);
    float3 gCameraPos : packoffset(c5);
    uint gNumLights : packoffset(c5.w);
    float3 gSunDirection : packoffset(c6);
}

cbuffer InstanceCB : register(b1)
{
    uint GeometryInfoIndex : packoffset(c0.x);
    uint VertexAttribIndex : packoffset(c0.y);
    uint IndexBufferIndex : packoffset(c0.z);
}
// Tempolar method, apply normal mapping later.
float3 UnpackNormalMap(Vertex v)
{
    float3 normal = v.normal;
    float3 tangeent = v.tangent;
    float3 biTangent = v.biTangent;
    
    return normal;
    
}

float BarycentricLerp(in float v0, in float v1, in float v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float2 BarycentricLerp(in float2 v0, in float2 v1, in float2 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float3 BarycentricLerp(in float3 v0, in float3 v1, in float3 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float4 BarycentricLerp(in float4 v0, in float4 v1, in float4 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

Vertex VertexBarycentricLerp(in Vertex v0, in Vertex v1, in Vertex v2, in float3 barycentrics)
{
    Vertex vtx;
    vtx.position = BarycentricLerp(v0.position, v1.position, v2.position, barycentrics);
    vtx.normal = normalize(BarycentricLerp(v0.normal, v1.normal, v2.normal, barycentrics));
    vtx.texCoord = BarycentricLerp(v0.texCoord, v1.texCoord, v2.texCoord, barycentrics);
    vtx.tangent = normalize(BarycentricLerp(v0.tangent, v1.tangent, v2.tangent, barycentrics));
    vtx.biTangent = normalize(BarycentricLerp(v0.biTangent, v1.biTangent, v2.biTangent, barycentrics));
    
    return vtx;
}

Vertex GetHitSurface(in BuiltInTriangleIntersectionAttributes attr, in uint vertexOffset, in uint indexOffset)
{
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    StructuredBuffer<Vertex> VertexBuffer = ResourceDescriptorHeap[VertexAttribIndex];
    Buffer<uint> IndexBuffer = ResourceDescriptorHeap[IndexBufferIndex];
    
    uint primIndex = PrimitiveIndex();
    
    uint i0 = IndexBuffer[indexOffset + primIndex * 3 + 0];
    uint i1 = IndexBuffer[indexOffset + primIndex * 3 + 1];
    uint i2 = IndexBuffer[indexOffset + primIndex * 3 + 2];
    
    Vertex v0 = VertexBuffer[vertexOffset + i0];
    Vertex v1 = VertexBuffer[vertexOffset + i1];
    Vertex v2 = VertexBuffer[vertexOffset + i2];

    return VertexBarycentricLerp(v0, v1, v2, barycentrics);
}

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
    
    RayDesc ray = GenerateCameraRay(launchIndex.xy, gCameraPos, gInvViewProj);

    ray.TMin = 0;
    ray.TMax = 100000;

    RayPayload payload;
    payload.color = float3(0, 0, 0);
    
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
    uint geometryIndex = GeometryIndex();
    
    StructuredBuffer<GeometryInfo> geoInfoBuffer = ResourceDescriptorHeap[GeometryInfoIndex];
    GeometryInfo geoInfo = geoInfoBuffer[geometryIndex];
    
    Vertex v = GetHitSurface(attribs, geoInfo.VertexOffset, geoInfo.IndexOffset);
    
    float hitT = RayTCurrent();
    float3 rayDirW = WorldRayDirection();
    float3 rayOriginW = WorldRayOrigin();
    
    // Find the world-space hit position
    float3 posW = rayOriginW + hitT * rayDirW;
    
    RayDesc ray;
    ray.Origin = posW;
    ray.Direction = gSunDirection;
    ray.TMin = 0.01;
    ray.TMax = 100000;
    
    ShadowPayload shadowPayload;
    uint traceRayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
    TraceRay(gRtScene, traceRayFlags, 0xFFFFFFFF, 1, 0, 1, ray, shadowPayload);
    
    float factor = shadowPayload.hit ? 0.1f : 1.0f;
    
    float3 albedo;
    if (geoInfo.AlbedoTextureIndex != UINT_MAX)
    {
        Texture2D<float3> albedoMap = ResourceDescriptorHeap[geoInfo.AlbedoTextureIndex];
        albedo = albedoMap.SampleLevel(gAnisotropicWrap, v.texCoord, 0.0f).xyz;
    }
    
    float metalic;
    if (geoInfo.MetalicTextureIndex != UINT_MAX)
    {
        Texture2D<float> metalicMap = ResourceDescriptorHeap[geoInfo.MetalicTextureIndex];
        metalic = metalicMap.SampleLevel(gAnisotropicWrap, v.texCoord, 0.0f).x;
    }
    
    float roughness;
    if (geoInfo.RoughnessTextureIndex != UINT_MAX)
    {
        Texture2D<float> roughnessMap = ResourceDescriptorHeap[geoInfo.RoughnessTextureIndex];
        roughness = roughnessMap.SampleLevel(gAnisotropicWrap, v.texCoord, 0.0f).x;
    }
    
    float3 color = albedo;
    
    //if(gNumLights > 0)
    //{
    //    StructuredBuffer<Light> light = ResourceDescriptorHeap[gLightIndex];
    //    color = DirectionalLightPBR(light[0], UnpackNormalMap(v), normalize(0.0f.xxx - WorldRayOrigin()), albedo, metalic, roughness);
    //}
    payload.color = color * factor;
}

[shader("anyhit")]
void AnyHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    const uint geometryIndex = GeometryIndex();
    
    StructuredBuffer<GeometryInfo> geoInfoBuffer = ResourceDescriptorHeap[GeometryInfoIndex];
    const GeometryInfo geoInfo = geoInfoBuffer[geometryIndex];
    
    const Vertex v = GetHitSurface(attribs, geoInfo.VertexOffset, geoInfo.IndexOffset);
    
    if (geoInfo.OpacityMapTextureIndex != UINT_MAX)
    {
        Texture2D opacityMap = ResourceDescriptorHeap[geoInfo.OpacityMapTextureIndex];
        if (opacityMap.SampleLevel(gAnisotropicWrap, v.texCoord, 0.0f).x < 0.35f)
        {
            IgnoreHit();
        }
    }
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
