uniform sampler2D uTex;
varying vec2 vTexCoord;

void main()
{
    gl_FragColor = texture2D(uTex, vTexCoord);
}
