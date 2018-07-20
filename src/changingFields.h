
/*
void randomField()
{
    E.x = .21; // to be randomized
    E.y = .0;
    E.z = .13;
}
*/

void changingFields(int parNo, int timeInd)
{
    float half = 0.5;
    
    if(s[parNo][timeInd].x <= half)
    {
        E.x = .21; // particle's electric field
    }
    else
    {
        E.x = -.25; // particle's initial electric field
    }
    
    if(s[parNo][timeInd].y <= half)
    {
        E.y = .09; // particle's electric field
    }
    else
    {
        E.y = -.11; // particle's initial electric field
    }
    
    if(s[parNo][timeInd].z <= half)
    {
        E.z = .49; // particle's electric field
    }
    else
    {
        E.z = -.46; // particle's initial electric field
    }
    return;
}
