#version 430

layout (location = 0) in vec4 _vp;

void main(void)
{
    gl_Position = _vp;
}