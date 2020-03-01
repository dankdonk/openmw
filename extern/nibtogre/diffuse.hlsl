float4x4 world_view_proj_matrix;
float4 Light_Ambient;
float4 Light1_Position;
float4x4 inv_world_matrix;
 
 
struct VS_OUTPUT 
{
   float4 Pos:      POSITION;
   float3 Color:        COLOR0;
};
 
 
VS_OUTPUT vs_main(float4 inPos: POSITION, float3 inNormal: NORMAL)
{
   VS_OUTPUT Out=(VS_OUTPUT)0;
 
   // Compute the projected position and send out the texture coordinates
   Out.Pos = mul(inPos,world_view_proj_matrix );
    
   // the normals might not be normalised (paraniod)
   inNormal=normalize(inNormal);
    
   // Start with the ambient color
   float4 Color =Light_Ambient;
 
 
   // Determine the light vector
   // first get the light vector in object space
   vector obj_light=mul(Light1_Position,inv_world_matrix);
   vector LightDir = normalize(obj_light - inPos);
 
 
   // Diffuse using Lambert
   float DiffuseAttn = max(0, dot(inNormal, LightDir) );
 
  
   // Compute final lighting
   // assume white light
   vector light={0.8,0.8,0.8,1};
   Color += light*DiffuseAttn;
 
    
 
   // Output Final Color
   Out.Color=Color;
    
   return Out;
}
 
float4 ps_main(float4 inColor: COLOR0) : COLOR 
{
    
   return inColor;
 
}
 
technique Diffuse
{
    pass P0
    {
        vertexShader = compile vs_2_0 vs_main();
        pixelShader = compile ps_2_0 ps_main();
    }
}
