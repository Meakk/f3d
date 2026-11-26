//VTK::System::Dec

//VTK::Output::Dec

in vec2 texCoord;

uniform sampler2D texColor;
uniform sampler2D texDepth;
uniform sampler2D texNormal;

uniform ivec2 resolution;
uniform float time;

//#define ENABLE_TONEMAPPING

// Khronos neutral tonemapping function
vec3 tonemap(vec3 color)
{
  const float startCompression = 0.8 - 0.04;
  const float desaturation = 0.15;
  float x = min(color.r, min(color.g, color.b));
  float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
  vec3 toned = color - vec3(offset);
  float peak = max(toned.r, max(toned.g, toned.b));
  if (peak >= startCompression)
  {
    const float d = 1. - startCompression;
    float newPeak = 1. - d * d / (peak + d - startCompression);
    toned *= newPeak / peak;
    float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
    toned = mix(toned, newPeak * vec3(1, 1, 1), g);
  }
  return toned;
}

vec4 getLinearColor(vec2 uv)
{
  vec4 color = texture(texColor, uv);
  if (color.a > 0.0)
    color.rgb /= color.a;

  // Convert color to linear space if needed
  //VTK::ColorSpace::Impl

#ifdef ENABLE_TONEMAPPING
  color.rgb = tonemap(color.rgb);
#endif

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
