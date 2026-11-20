// Native mesh coverage is destination-pixel checkerboard, not source alpha.
bool lagiVdp1MeshDiscard(float x,float y)
{
    return frac((floor(x)+floor(y))*0.5f)>=0.5f;
}
