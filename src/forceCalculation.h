
// dynamic force calculation
float Fx(float vx, float vy, float vz, float Ex, float Ey, float Ez, float Bx, float By, float Bz)
{
    return Ex + vy*Bz - vz*By;
}

float Fy(float vx, float vy, float vz, float Ex, float Ey, float Ez, float Bx, float By, float Bz)
{
    return Ey + vz*Bx - vx*Bz;
}

float Fz(float vx, float vy, float vz, float Ex, float Ey, float Ez, float Bx, float By, float Bz)
{
    return Ez + vx*By - vy*Bx;
}
