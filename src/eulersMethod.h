
// updating position values
void updatePosition(int parNo, int timeInd)
{
    s[parNo][timeInd+1].x = s[parNo][timeInd].x + timeChange * v[parNo][timeInd].x;
    s[parNo][timeInd+1].y = s[parNo][timeInd].y + timeChange * v[parNo][timeInd].y;
    s[parNo][timeInd+1].z = s[parNo][timeInd].z + timeChange * v[parNo][timeInd].z;
    
    return;
}

// updating velocity values
void updateVelocity(int parNo, int timeInd)
{
    v[parNo][timeInd+1].x = v[parNo][timeInd].x + timeChange * Fx(v[parNo][timeInd].x, v[parNo][timeInd].y, v[parNo][timeInd].z, E.x, E.y, E.z, B.x, B.y, B.z);
    v[parNo][timeInd+1].y = v[parNo][timeInd].y + timeChange * Fy(v[parNo][timeInd].x, v[parNo][timeInd].y, v[parNo][timeInd].z, E.x, E.y, E.z, B.x, B.y, B.z);
    v[parNo][timeInd+1].z = v[parNo][timeInd].z + timeChange * Fz(v[parNo][timeInd].x, v[parNo][timeInd].y, v[parNo][timeInd].z, E.x, E.y, E.z, B.x, B.y, B.z);
    
    return;
}
