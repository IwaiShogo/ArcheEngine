// 定数バッファ (Transformなど)
cbuffer ConstantBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 LightDir;
	float4 LightColor;
	float4 MaterialColor;
	
	matrix BoneTransforms[100];
	int HasAnimation;
	float3 padding;
}

// テクスチャ
Texture2D tex : register(t0);
SamplerState sam : register(s0);

struct VS_INPUT
{
	float4 Pos : POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	int4 boneIds : BONE_IDS;
	float4 weights : WEIGHTS;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
};

// Vertex Shader
PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT) 0;
	
	float4 localPos = input.Pos;
	float3 localNormal = input.Normal;

	// スキニング計算
	if (HasAnimation != 0)
	{
		float4 totalPos = float4(0, 0, 0, 0);
		float3 totalNormal = float3(0, 0, 0);

		for (int i = 0; i < 4; i++)
		{
			if (input.boneIds[i] < 0 || input.boneIds[i] >= 100)
				continue;

			matrix boneM = BoneTransforms[input.boneIds[i]];
			
			float4 posePos = mul(input.Pos, boneM);
			totalPos += posePos * input.weights[i];
			
			float3 poseNormal = mul(input.Normal, (float3x3) boneM);
			totalNormal += poseNormal * input.weights[i];
		}
		
		if (totalPos.w > 0.0f)
		{
			localPos = totalPos;
			localNormal = normalize(totalNormal);
			localPos.w = 1.0f;
		}
	}

	output.Pos = mul(localPos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	
	output.Normal = mul(localNormal, (float3x3) World);
	output.Normal = normalize(output.Normal);
	output.UV = input.UV;
	
	return output;
}

// Pixel Shader
float4 PS(PS_INPUT input) : SV_Target
{
	float4 texColor = tex.Sample(sam, input.UV) * MaterialColor;

	float3 L = normalize(-LightDir.xyz);
	float3 N = normalize(input.Normal);
	
	float diff = dot(N, L) * 0.5 + 0.5;
	float3 ambient = float3(0.3, 0.3, 0.3);
	float3 diffuse = diff * LightColor.rgb;
	
	float3 finalColor = texColor.rgb * (diffuse + ambient);
	
	return float4(finalColor, texColor.a);
}