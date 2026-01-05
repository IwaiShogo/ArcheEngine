// 定数バッファ (Transformなど)
cbuffer ConstantBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 LightDir; // ライトの方向 (xyz)
	float4 LightColor; // ライトの色
	float4 MaterialColor; // 物体の色
}

// テクスチャ
Texture2D tex : register(t0);
SamplerState sam : register(s0);

struct VS_INPUT
{
	float4 Pos	  : POSITION;
	float3 Normal : NORMAL; // 法線
	float2 UV	  : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Pos	  : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UV	  : TEXCOORD0;
};

// Vertex Shader
PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	
	// 座標変換
	output.Pos = mul(input.Pos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	
	// 法線の変換 (回転のみ適用するため、Worldの逆転置行列を使うのが正式ですが、
	// 均等スケーリングならWorldで代用可。ここでは簡易的にWorldを使います)
	output.Normal = mul(input.Normal, (float3x3)World);
	output.Normal = normalize(output.Normal); // 正規化
	
	output.UV = input.UV;
	
	return output;
}

// Pixel Shader
float4 PS(PS_INPUT input) : SV_Target
{
	// 1. テクスチャ色 (なければ白)
    float4 texColor = tex.Sample(sam, input.UV) * MaterialColor;
	
	// 2. ライト計算
	// 光の方向を正規化（念のため）
    float3 L = normalize(-LightDir.xyz);
    float3 N = normalize(input.Normal);
	
	// ランバート反射 (0.0 ~ 1.0)
	// ハーフランバート (0.5 ~ 1.0) にすると陰影が柔らかくなり、真っ黒になりにくいです
    float diff = max(dot(N, L), 0.0);
	// float diff = dot(N, L) * 0.5 + 0.5; // ハーフランバートを試すならこちら

	// 環境光 (強めにしてみる)
    float3 ambient = float3(0.5, 0.5, 0.5);
	
	// 拡散光
    float3 diffuse = diff * LightColor.rgb;
	
	// 最終合成
    float3 finalColor = texColor.rgb * (diffuse + ambient);
	
    return float4(finalColor, texColor.a);
}