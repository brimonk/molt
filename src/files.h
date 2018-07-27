
void fileManipulation(int timeInd, int loopCount, int parNo, int noPar)
{
    FILE *xPos, *yPos, *zPos, *xVel, *yVel, *zVel;

    xPos = fopen("./xPos.csv", "w"); // opening the files
    yPos = fopen("./yPos.csv", "w");
    zPos = fopen("./zPos.csv", "w");

    xVel = fopen("./xVel.csv", "w");
    yVel = fopen("./yVel.csv", "w");
    zVel = fopen("./zVel.csv", "w");

    timeInd = 0;
    for(parNo=0; parNo < noPar; parNo++) // prints the initial conditions
    {
        fprintf(xPos, "%.3f, ", s[parNo][timeInd].x); // writing the particles' intial position to file
        fprintf(yPos, "%.3f, ", s[parNo][timeInd].y);
        fprintf(zPos, "%.3f, ", s[parNo][timeInd].z);

        fprintf(xVel, "%.3f, ", v[parNo][timeInd].x); // writing the particles' initial velocity to file
        fprintf(yVel, "%.3f, ", v[parNo][timeInd].y);
        fprintf(zVel, "%.3f, ", v[parNo][timeInd].z);
    }

    fprintf(xPos, "\n");
    fprintf(yPos, "\n");
    fprintf(zPos, "\n");

    fprintf(xVel, "\n");
    fprintf(yVel, "\n");
    fprintf(zVel, "\n");

    for(timeInd=0; timeInd < loopCount; timeInd++) // writing data to file
    {
        for(parNo=0; parNo < noPar; parNo++)
        {
            fprintf(xPos, "%.3f, ", s[parNo][timeInd+1].x); // writing the particles' positions to file
            fprintf(yPos, "%.3f, ", s[parNo][timeInd+1].y);
            fprintf(zPos, "%.3f, ", s[parNo][timeInd+1].z);

            fprintf(xVel, "%.3f, ", v[parNo][timeInd+1].x); // writing the particles' velocities to file
            fprintf(yVel, "%.3f, ", v[parNo][timeInd+1].y);
            fprintf(zVel, "%.3f, ", v[parNo][timeInd+1].z);
        }
        fprintf(xPos, "\n");
        fprintf(yPos, "\n");
        fprintf(zPos, "\n");

        fprintf(xVel, "\n");
        fprintf(yVel, "\n");
        fprintf(zVel, "\n");
    }

    fclose(xPos); // closing the files
    fclose(yPos);
    fclose(zPos);

    fclose(xVel);
    fclose(yVel);
    fclose(zVel);

    return;
}
