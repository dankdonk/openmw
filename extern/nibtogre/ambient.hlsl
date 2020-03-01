//uniform variables
float4x4 world_view_proj_matrix; //world, view & projection matrices combined
float4 Light_Ambient; //colour of ambient light
 
 
 
struct VS_OUTPUT //the vertex shader (VS) will return this struct
{
   float4 Pos:      POSITION;
   float3 Color:    COLOR0;
};
 
//vertex shader
VS_OUTPUT vs_main(float4 inPos: POSITION) 
{
   VS_OUTPUT Out=(VS_OUTPUT)0;
 
   // Compute the projected position and send out the texture coordinates
   Out.Pos = mul(inPos,world_view_proj_matrix );
    
    
    
   // Start with the ambient color
   float4 Color = Light_Ambient;
  
 
   // Output Final Color
   Out.Color=Color;
    
   return Out;
}
 
//pixel shader
float4 ps_main(float4 inColor: COLOR0) : COLOR 
{
    
   return inColor;
 
}
 
technique Ambient
{
    pass P0
    {
        vertexShader = compile vs_2_0 vs_main();
        pixelShader = compile ps_2_0 ps_main();
    }
}
