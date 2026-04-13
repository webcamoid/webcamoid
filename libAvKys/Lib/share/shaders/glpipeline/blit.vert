attribute vec4 aPos;
attribute vec2 aTexCoord;
varying vec2 vTexCoord;

void main()
{
    gl_Position = aPos;
    vTexCoord = aTexCoord;
}
