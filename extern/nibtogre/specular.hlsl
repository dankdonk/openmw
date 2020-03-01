VS_OUTPUT vs_main(float4 inPos: POSITION, float3 inNormal: NORMAL)
{
   VS_OUTPUT Out=(VS_OUTPUT)0;
 
   // Compute the projected position and send out the texture coordinates
   Out.Pos = mul(inPos,view_proj_matrix );
    
   // the normals might not be normalised
   inNormal=normalize(inNormal);
    
   // Start with the ambient color
   float4 Color =Light_Ambient;
 
 
    
    
  
   // Determine the light vector
   // first get the light vector in object space
   vector obj_light=mul(Light1_Position,inv_world_matrix);
   vector LightDir = normalize(obj_light - inPos);
 
 
   // Determine the eye vector
   // first get the eye vector in object space
   vector obj_eye=mul(view_position,inv_world_matrix);
   vector EyeDir = normalize(obj_eye-inPos);
 
   // Compute half vector
   vector HalfVect = normalize((LightDir+EyeDir)/2);
 
    
 
   // Specular, using Blinn Phong and a 'shinesiness' value of 64
   float SpecularAttn =  max(0,pow(  dot(inNormal, HalfVect),64));
   
 
 
   // Diffuse using Lambert
   float DiffuseAttn = max(0, dot(inNormal, LightDir) );
   
  
   // Compute final lighting
   // assume white light
   vector light={1,1,1,1};
   Color +=  light*SpecularAttn+light*DiffuseAttn;
 
    
 
   // Output Final Color
   Out.Color=Color;
    
   return Out;
}
