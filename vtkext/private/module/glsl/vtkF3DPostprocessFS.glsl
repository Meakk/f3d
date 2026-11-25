//VTK::System::Dec

//VTK::Output::Dec

in vec2 texCoord;

uniform sampler2D texLinearHDR;
uniform sampler2D texDepth;
uniform sampler2D texNormal;

uniform ivec2 resolution;
uniform float time;

vec4 getLinearColor(vec2 uv)
{
  vec4 color = texture(texLinearHDR, uv);

  // TODO: FXAA, tone mapping

  return color;
}

vec4 toSRGB(vec4 color)
{
  return vec4(pow(color.rgb, vec3(1.0/2.2)), color.a);
}

// placeholder
// TODO: replace by user input 
//VTK::User::Dec
vec4 pixel(vec2 uv)
{
  return getLinearColor(uv);
}
    
void main()
{
  gl_FragData[0] = toSRGB(pixel(texCoord));
}
