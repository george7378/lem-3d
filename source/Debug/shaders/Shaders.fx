////////////////////
//Global variables//
////////////////////
float4x4 m_World;
float4x4 m_WorldViewProjection;
float4x4 m_LightWorldViewProjection;
float4x4 m_DustWorldViewProjection;

float f_AmbientStrength;
float f_DiffuseStrength;
float f_SpecularPower;
float f_Reflectivity;
float f_ShadowBias;
float f_DustFactor;

float3 v3_LightDir;
float3 v3_CamPos;

bool b_ReceiveShadows;

Texture t_Texture;
Texture t_TextureN;
Texture t_TextureS;
Texture t_ShadowMap;
Texture t_MoonDetailSmall;
Texture t_DustBlast;


//////////////////
//Sampler states//
//////////////////
sampler TextureSampler = sampler_state 
{
	texture = <t_Texture>;
	magfilter = LINEAR;
	minfilter = LINEAR;
	mipfilter = LINEAR;
	AddressU = Wrap; 
	AddressV = Wrap;
};

sampler NormalMapSampler = sampler_state 
{
	texture = <t_TextureN>;
	magfilter = LINEAR;
	minfilter = LINEAR;
	mipfilter = LINEAR;
	AddressU = Wrap; 
	AddressV = Wrap;
};

sampler SpecularMapSampler = sampler_state 
{
	texture = <t_TextureS>;
	magfilter = LINEAR;
	minfilter = LINEAR;
	mipfilter = LINEAR;
	AddressU = Wrap; 
	AddressV = Wrap;
};

sampler ShadowMapSampler = sampler_state 
{
	texture = <t_ShadowMap>;
	magfilter = LINEAR;
	minfilter = LINEAR;
	mipfilter = LINEAR;
	AddressU = Border; 
	AddressV = Border;
	BorderColor = float4(1, 1, 1, 1);
};

sampler MoonDetailSmallSampler = sampler_state 
{
	texture = <t_MoonDetailSmall>;
	magfilter = LINEAR;
	minfilter = LINEAR;
	mipfilter = LINEAR;
	AddressU = Wrap; 
	AddressV = Wrap;
};

sampler DustBlastSampler = sampler_state 
{
	texture = <t_DustBlast>;
	magfilter = LINEAR;
	minfilter = LINEAR;
	mipfilter = LINEAR;
	AddressU = Border; 
	AddressV = Border;
	BorderColor = float4(0, 0, 0, 0);
};


//////////////////
//I/O structures//
//////////////////
struct PixelColourOut
{
    float4 Colour        : COLOR0;
};

struct ShadowMapVertexToPixel
{
    float4 Position      : POSITION;
    float Depth   		 : TEXCOORD0;
};

struct NMVertexToPixel
{
    float4 Position	   	 : POSITION;
    float2 TexCoords     : TEXCOORD0;
    float4 Position3D    : TEXCOORD1;
    float3 LightDirTS	 : TEXCOORD2;
    float3 ViewDirTS	 : TEXCOORD3;
    float4 Pos2DLight    : TEXCOORD4;
};

struct TerrainVertexToPixel
{
    float4 Position	   	 : POSITION;
    float2 TexCoords     : TEXCOORD0;
    float4 Pos2DLight    : TEXCOORD1;
    float4 Pos2DDust     : TEXCOORD2;
    float3 Normal        : TEXCOORD3;
};

struct EmissiveVertexToPixel
{
    float4 Position	   	 : POSITION;
    float2 TexCoords     : TEXCOORD0;
};


////////////////////////////////////////////////
//TECHNIQUE 1: Shaders for making a shadow map//
////////////////////////////////////////////////
ShadowMapVertexToPixel ShadowMapVertexShader(float4 inPos : POSITION)
{
    ShadowMapVertexToPixel Output = (ShadowMapVertexToPixel)0;

    Output.Position = mul(inPos, m_LightWorldViewProjection);
    Output.Depth = Output.Position.z;

    return Output;
}

PixelColourOut ShadowMapPixelShader(ShadowMapVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;            

    Output.Colour = float4(PSIn.Depth, PSIn.Depth, PSIn.Depth, 1);

    return Output;
}


technique ShadowMap
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 ShadowMapVertexShader();
        PixelShader = compile ps_2_0 ShadowMapPixelShader();
    }
}


///////////////////////////////////////////////////
//TECHNIQUE 2: Shaders for a normal mapped object//
///////////////////////////////////////////////////
NMVertexToPixel NMVertexShader(float4 inPos : POSITION, float2 inTexCoords : TEXCOORD0, float3 inNormal : NORMAL, float3 inTangent : TANGENT, float3 inBinormal : BINORMAL) 
{
    NMVertexToPixel Output = (NMVertexToPixel)0;

    Output.Position = mul(inPos, m_WorldViewProjection);
    Output.TexCoords = inTexCoords;
    
    float3x3 worldToTangentSpace;
    worldToTangentSpace[0] = mul(inTangent, (float3x3)m_World);
    worldToTangentSpace[1] = mul(inBinormal, (float3x3)m_World);
    worldToTangentSpace[2] = mul(inNormal, (float3x3)m_World);
    
    Output.Position3D = mul(inPos, m_World);
    
    Output.LightDirTS = mul(worldToTangentSpace, v3_LightDir);
    Output.ViewDirTS = mul(worldToTangentSpace, v3_CamPos - Output.Position3D.xyz);
    
    Output.Pos2DLight = mul(inPos, m_LightWorldViewProjection);
    
    return Output;
}

PixelColourOut NMPixelShader(NMVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;
    
    float4 baseColour = tex2D(TextureSampler, PSIn.TexCoords);
    float3 normal = normalize(2*tex2D(NormalMapSampler, PSIn.TexCoords).xyz - 1);
    
    float2 projTexCoord;
    projTexCoord[0] = PSIn.Pos2DLight.x/PSIn.Pos2DLight.w/2 + 0.5f;
    projTexCoord[1] = -PSIn.Pos2DLight.y/PSIn.Pos2DLight.w/2 + 0.5f;
    
    float diffuseLightingFactor = saturate(dot(normalize(-PSIn.LightDirTS), normal))*f_DiffuseStrength;

    float3 reflectionVector = normalize(reflect(PSIn.LightDirTS, normal));
    float specularLightingFactor = pow(saturate(dot(reflectionVector, normalize(PSIn.ViewDirTS))), f_SpecularPower)*f_Reflectivity;
    
    if (b_ReceiveShadows)
    {
    	if (tex2D(ShadowMapSampler, projTexCoord).r < PSIn.Pos2DLight.z - f_ShadowBias)
    	{
    		diffuseLightingFactor = 0;
    		specularLightingFactor = 0;
		}
	}
	
    Output.Colour = baseColour*(diffuseLightingFactor + f_AmbientStrength) + float4(float3(1, 1, 1)*specularLightingFactor, 1);

    return Output;
}


technique NormalMap
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 NMVertexShader();
        PixelShader = compile ps_2_0 NMPixelShader();
    }
}


////////////////////////////////////
//TECHNIQUE 3: Shaders for terrain//
////////////////////////////////////
TerrainVertexToPixel TerrainVertexShader(float4 inPos : POSITION, float2 inTexCoords : TEXCOORD0, float3 inNormal : NORMAL) 
{
    TerrainVertexToPixel Output = (TerrainVertexToPixel)0;

    Output.Position = mul(inPos, m_WorldViewProjection);
    Output.TexCoords = inTexCoords;
    
    Output.Pos2DLight = mul(inPos, m_LightWorldViewProjection);
    Output.Pos2DDust = mul(inPos, m_DustWorldViewProjection);
    
    Output.Normal = normalize(mul(inNormal, (float3x3)m_World));
    
    return Output;
}

PixelColourOut TerrainPixelShader(TerrainVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;
    
    float4 baseColour = tex2D(TextureSampler, PSIn.TexCoords);
    baseColour *= tex2D(MoonDetailSmallSampler, PSIn.TexCoords*20.5f)*2;   
    
    float diffuseLightingFactor = saturate(dot(normalize(-v3_LightDir), PSIn.Normal))*f_DiffuseStrength;
    
    float2 shadowProjTexCoord;
    shadowProjTexCoord[0] = PSIn.Pos2DLight.x/PSIn.Pos2DLight.w/2 + 0.5f;
    shadowProjTexCoord[1] = -PSIn.Pos2DLight.y/PSIn.Pos2DLight.w/2 + 0.5f;
    
    if (tex2D(ShadowMapSampler, shadowProjTexCoord).r < 1)
    {
    	diffuseLightingFactor = 0;
	}
	
	float2 dustProjTexCoord;
    dustProjTexCoord[0] = PSIn.Pos2DDust.x/PSIn.Pos2DDust.w/2 + 0.5f;
    dustProjTexCoord[1] = -PSIn.Pos2DDust.y/PSIn.Pos2DDust.w/2 + 0.5f;
    
    float4 dustColour = tex2D(DustBlastSampler, dustProjTexCoord);
    
    baseColour = lerp(baseColour, dustColour, dustColour.a*f_DustFactor);

    Output.Colour = baseColour*(diffuseLightingFactor + f_AmbientStrength);

    return Output;
}


technique Terrain
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 TerrainVertexShader();
        PixelShader = compile ps_2_0 TerrainPixelShader();
    }
}

