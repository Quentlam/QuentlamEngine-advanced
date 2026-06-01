#type vertex
#version 410 core

out vec2 v_TexCoord;

void main()
{
    vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    v_TexCoord = pos;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}

#type fragment
#version 410 core

layout(location = 0) out vec4 o_Color;

in vec2 v_TexCoord;

uniform isampler2D u_EntityIDTexture;
uniform int u_SelectedEntity;
uniform vec4 u_OutlineColor;
uniform int u_OutlineWidth;
uniform vec2 u_TexSize;
uniform float u_OutlineIntensity;

int getMask(vec2 offset)
{
    return texture(u_EntityIDTexture, v_TexCoord + offset * u_TexSize).r;
}

void main()
{
    int center = getMask(vec2(0.0));

    float w = float(u_OutlineWidth);

    // Build a mask: pixels that belong to the selected entity (center) or any of its neighbors
    bool centerSelected = (center == u_SelectedEntity);
    bool neighborOfSelected = false;

    // Sample all 8 neighbors; if any is the selected entity, this pixel is near the selection boundary
    neighborOfSelected = neighborOfSelected || (getMask(vec2(-w, -w)) == u_SelectedEntity);
    neighborOfSelected = neighborOfSelected || (getMask(vec2(-w,  0.0)) == u_SelectedEntity);
    neighborOfSelected = neighborOfSelected || (getMask(vec2(-w,  w)) == u_SelectedEntity);
    neighborOfSelected = neighborOfSelected || (getMask(vec2( w, -w)) == u_SelectedEntity);
    neighborOfSelected = neighborOfSelected || (getMask(vec2( w,  0.0)) == u_SelectedEntity);
    neighborOfSelected = neighborOfSelected || (getMask(vec2( w,  w)) == u_SelectedEntity);
    neighborOfSelected = neighborOfSelected || (getMask(vec2( 0.0, -w)) == u_SelectedEntity);
    neighborOfSelected = neighborOfSelected || (getMask(vec2( 0.0,  w)) == u_SelectedEntity);

    // Outline pixel if: (center is selected AND neighbor differs) OR (center differs BUT neighbor is selected)
    bool edge = (centerSelected && !neighborOfSelected) || (!centerSelected && neighborOfSelected);

    if (edge)
    {
        float alpha = clamp(float(u_OutlineIntensity) * u_OutlineColor.a, 0.0, 1.0);
        o_Color = vec4(u_OutlineColor.rgb, alpha);
    }
    else
    {
        discard;
    }
}
