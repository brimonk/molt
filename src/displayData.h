
void displayInitialParameters(int parNo, int noPar, int timeInd)
{
    for(parNo=1; parNo<(noPar+1); parNo++) // prints the initial conditions
    {
        printf("\n [%i] p%i. (%.3f, %.3f, %.3f)", timeInd+1, parNo, s[parNo][timeInd].x, s[parNo][0].y, s[parNo][0].z);
        printf("\t(%.3f, %.3f, %.3f)", v[parNo][timeInd].x, v[parNo][0].y, v[parNo][0].z);
    }
    
    return;
}

void displayUpdatedData(int parNo, int noPar, int timeInd)
{
    printf("\n [%i] p%i. (%.3f, %.3f, %.3f)", timeInd+2, parNo, s[parNo][timeInd+1].x, s[parNo][timeInd+1].y, s[parNo][timeInd+1].z);
    printf("\t(%.3f, %.3f, %.3f)", v[parNo][timeInd+1].x, v[parNo][timeInd+1].y, v[parNo][timeInd+1].z);
     
    return;
}
