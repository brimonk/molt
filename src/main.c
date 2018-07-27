
//  Force on a point charge
//
//  Created by Khari Gray on 4/16/18.
//  Copyright © 2018 Khari Gray. All rights reserved.

#include "structures.h"

// necessary prototypes
void updatePosition(int, int);
void updateVelocity(int, int);
void displayInitialParameters(int, int, int);
struct vector vectorize(float, float, float);
float Fx(float, float, float, float, float, float, float, float, float);
float Fy(float, float, float, float, float, float, float, float, float);
float Fz(float, float, float, float, float, float, float, float, float);

// declaring global variables
float timeInitial, timeFinal, timeChange, currTime;
struct vector s[999][999], v[999][999], E, B; // position, velocity, electric field, magnetic field

#include <time.h> // innate C libraries
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// user defined libraries
#include "files.h" // funcitons that relate to writing all the data to file to be read and plotted by Matlab
#include "displayData.h" // functions the display the data to the user in the console
#include "eulersMethod.h" // functions that perform iterative techniques on position and velocity
#include "miscellaneous.h" // simple functions, doesn't affect the project's idea
#include "changingFields.h" // changing field
#include "forceCalculation.h" // functinos to calculate the force
#include "initialParameters.h" // functions that accept the intiial condition for the simulation

int main()
{
    int parNo, noPar, loopCount, timeInd = 0; // timeInd indicates the iteration in time that the simulation is at
    
    noPar = initialParameters(); // reading the initial parameters
    parNo = 1;
    tableHeading(); // printing the heading for the table (position .. velocity)
    displayInitialParameters(parNo, noPar, timeInd);  // displaying the initial parameters for all the particles
    
    while(currTime < timeFinal) // plotting the position for each particle at each time step
    {
        printNewLine();
        for(parNo=1; parNo<(noPar+1); parNo++) // repeating funtions per number of particles
        {
            
            changingFields(parNo, timeInd); // dynamizing the fields
            updatePosition(parNo, timeInd); // updating position values
            updateVelocity(parNo, timeInd); // updating velocity values
            displayUpdatedData(parNo, noPar, timeInd); // displaying the updated position and velocity data
            if(parNo == noPar) // iterates to the next time step when all data has been collected for all particles for that instance
            {
                timeInd++;
            }
        }
        currTime = currTime + timeChange; // incrementing time value
    }
    loopCount = timeInd;
    
    fileManipulation(timeInd, loopCount, parNo, noPar); // writes all the data to .csv files, to be later read and plotted by Matlab
    closingStatement(); // printing an exit statement for the user
    
    return 0;
}
