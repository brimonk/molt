
// NOTE: ALL UNITS ARE TO BE ASSUMED BASE UNITS
int initialParameters()
{
    int noPar, parNo, timeInd = 0;
    float maxPos, maxE, maxB, halfPlane = .5; // max... tells the maximum values of the displacements and E and B fields
    
    timeInitial = 0; // time simulation start
    timeFinal = 1; // time simulaion ends
    timeChange = 1; // time increment
    currTime = timeInitial;
    noPar = 5; // number of particles being simulated
    
    srand((unsigned int)time(NULL)); // generates a random number
    maxPos = 1; // maximum displacement of the particle from origin
    maxE = 5; // maximum electric field
    maxB = 5; // maximum magnetic field
    
    for(parNo = 0; parNo < noPar; parNo++) // sets the initial conditions for all particles
    {
        s[parNo][timeInd].x = ((float)rand()/(float)(RAND_MAX)) * maxPos; // particle's initial position
        s[parNo][timeInd].y = ((float)rand()/(float)(RAND_MAX)) * maxPos;
        s[parNo][timeInd].z = parNo;
        
        v[parNo][timeInd].x = ((float)rand()/(float)(RAND_MAX)) * maxPos; // particle's initial velocity
        v[parNo][timeInd].y = 0.92;
        v[parNo][timeInd].z = ((float)rand()/(float)(RAND_MAX)) * maxPos;
    }

    if(s[parNo][timeInd].x > halfPlane) // if the particle has crossed the half way plane in the x direction
    {
        E.x = s[parNo][timeInd].x; // particle's electric field
        E.y = .01;
        E.z = ((float)rand()/(float)(RAND_MAX)) * maxE;
    }
    else
    {
        E.x = -s[parNo][timeInd].x; // particle's initial electric field
        E.y = ((float)rand()/(float)(RAND_MAX)) * maxE;
        E.z = ((float)rand()/(float)(RAND_MAX)) * maxE;
    }

    B.x = .32; // particle's initial magnetic field
    B.y = .20;
    B.z = .89;
    
    return noPar;
}
