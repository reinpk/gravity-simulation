/* GravitySimulation  --- main.cpp
 *
 *Peter K. Reinhardt - 2008
 *This code may not be distributed without the authors explicit written permission.
 */

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <math.h>

#include "simulation.h"

using namespace std;

int main()
{
	ofstream fout;
	fout.open("dataIn.txt", ios_base::app|ios_base::out);
	ifstream fin ("inputState.txt");

	int useExitState;
	fin >> useExitState;

	int totalFrames = 50000;
	int ustepsize = 100;
	Field field;

	if (useExitState == 1) /*Use the exit state of a previous sim as the starting point*/ {
		int frames;
		fin >> field.particleCount >> field.initialBounds >> field.timeStep >> ustepsize >> frames >> field.localized >> field.boundarysmoothing;
		printf("Using an exit state as a starting point: %i particles, %f bounds... using %i threads.\n", field.particleCount, field.initialBounds, NUM_THREADS);
		for (int j = 0; j < field.particleCount; j++) {
			fin >> field.particles[j].exists >> field.particles[j].mass >> field.particles[j].particleRadius;
			fin >> field.particles[j].position[0] >> field.particles[j].position[1] >> field.particles[j].position[2];
			fin >> field.particles[j].velocity[0] >> field.particles[j].velocity[1] >> field.particles[j].velocity[2];
			
			//now initialize all the other stuff
			field.particles[j].tempPosition[0] = field.particles[j].position[0];
			field.particles[j].tempPosition[1] = field.particles[j].position[1];
			field.particles[j].tempPosition[2] = field.particles[j].position[2];

			field.particles[j].acceleration[0] = 0;
			field.particles[j].acceleration[1] = 0;
			field.particles[j].acceleration[2] = 0;
		
			for (int i = 0; i < 4; i++) {
				field.particles[j].ka[i][0] = 0;
				field.particles[j].ka[i][1] = 0;
				field.particles[j].ka[i][2] = 0;

				field.particles[j].kv[i][0] = 0;
				field.particles[j].kv[i][1] = 0;
				field.particles[j].kv[i][2] = 0;
			}
		}
	} else /*Start a new sim*/{
		field.initialBounds = 5000;
		field.timeStep = 0.5;
		field.particleCount = 3000;
		field.localized = true;
		field.boundarysmoothing = false;
		field.initVelocitySpread = 0.01;
		field.massAverage = 1500;
		field.massSpread = 500;
		
		printf("Starting a new simulation: %i particles, %f bounds... using %i threads.\n", field.particleCount, field.initialBounds, NUM_THREADS);

		initParticles(&field);
	}
	getCenterOfMass(&field);
	double timeStep = field.timeStep;

	if (field.particleCount > MAX_PARTICLES) return -1;
	
	//the first number specifies the type of file output
	fout << "0 " << field.particleCount << " " << field.initialBounds << " " << (totalFrames/ustepsize) << endl;
	fout << field.COM[0] << " " << field.COM[1] << " " << field.COM[2] << endl;

	//start clocks for giving finish time estimates
	clock_t start_time = clock();
	clock_t cur_time;
	
	//initialize all the multithreaded objects
	if (MULTITHREADED) {
		//create mutexes for each particle
		for (int i = 0; i < field.particleCount; i++) {
			particleMutexes[i] = CreateMutex(NULL, FALSE, NULL);
		}
		
		//make the event that triggers the go ahead for another runge kutta k calculation
		hEventGo[0] = CreateEvent( 
			NULL,               // default security attributes
			TRUE,               // manual-reset event
			FALSE,              // initial state is signaled
			NULL				// object name
		);

		//make the threads, events and holders
		for (int t = 0; t < NUM_THREADS; t++) {
			hEventDone[t] = CreateEvent(NULL, TRUE, FALSE, NULL);
			holders[t].field = &field;
			holders[t].id = t;
			threads[t] = CreateThread(NULL, 0, calculateRungeKuttaK_MT, &holders[t], 0, &dwThreadId[t]);
		}
	}

	for (int i = 0; i < totalFrames/ustepsize; i++) {
		for (int j = 0; j < ustepsize; j++) {
			//choose an integration method
			if (EULER) {
				incrementPosition(&field);
			} else /*RUNGE KUTTA*/ {
				for (int k = 0; k < 4; k++) {
					//choose a threading option
					if (MULTITHREADED) {
						//signal that the threads can go ahead
						SetEvent(hEventGo[0]);
						//now wait for the threads to return
						WaitForMultipleObjects(2, hEventDone, TRUE, INFINITE);
						for (int t = 0; t < NUM_THREADS; t++) {
							ResetEvent(hEventDone[t]);
						}
					} else /*SINGLE THREADED*/ {
						calculateRungeKuttaK(&field, k);
					}
					incrementTemporaryState(&field, k);
				}
				incrementStateRungeKutta(&field);
				resetRungeKuttaK(&field);
			}
		} //j for

		/*getCenterOfMass(&field);
		fout << field.COM[0] << " " << field.COM[1] << " " << field.COM[2] << endl;*/

		for (int j = 0; j < field.particleCount; j++) {
			fout << field.particles[j].exists << " " << field.particles[j].position[0] << " " << field.particles[j].position[1] << " " << field.particles[j].position[2] << " " << field.particles[j].mass << endl;
		}

		//write out the current exit state
		ofstream foutexit ("exitState.txt");
		foutexit << "1 " << field.particleCount << " " << field.initialBounds << " " << timeStep << " " << ustepsize << " " << (totalFrames/ustepsize) << " " << field.localized << " " << field.boundarysmoothing << endl;
		for (int j = 0; j < field.particleCount; j++) {
			foutexit << field.particles[j].exists << " ";
			foutexit << field.particles[j].mass << " ";
			foutexit << field.particles[j].particleRadius << " ";
			foutexit << field.particles[j].position[0] << " ";
			foutexit << field.particles[j].position[1] << " ";
			foutexit << field.particles[j].position[2] << " ";
			foutexit << field.particles[j].velocity[0] << " ";
			foutexit << field.particles[j].velocity[1] << " ";
			foutexit << field.particles[j].velocity[2] << endl;
		}
		foutexit.close();
		
		//estimate the completion time
		cur_time = clock();
		int seconds = (int)(((double)(cur_time - start_time)/CLOCKS_PER_SEC)*((double)totalFrames/((double)ustepsize*(i+1)) - 1.0));
		int min = 0;
		int hour = 0;
		int day = 0;
		if (seconds > 60) {
			min = seconds/60;
			seconds %= 60;
		}
		if (min > 60) {
			hour = min/60;
			min %= 60;
		}
		if (hour > 24) {
			day = hour/24;
			hour %= 24;
		}

		printf("%i out of %i frames done.... %i d, %i h, %i min, %i s left.\n", (i+1)*ustepsize, totalFrames, day, hour, min, seconds);
	} //i for

	getCenterOfMass(&field);
	fout << field.COM[0] << " " << field.COM[1] << " " << field.COM[2] << endl;

	//write out the exit state
	ofstream foutexit ("exitState.txt");
	foutexit << "1 " << field.particleCount << " " << field.initialBounds << " " << timeStep << " " << ustepsize << " " << (totalFrames/ustepsize) << " " << field.localized << " " << field.boundarysmoothing << endl;
	for (int j = 0; j < field.particleCount; j++) {
		foutexit << field.particles[j].exists << " ";
		foutexit << field.particles[j].mass << " ";
		foutexit << field.particles[j].particleRadius << " ";
		foutexit << field.particles[j].position[0] << " ";
		foutexit << field.particles[j].position[1] << " ";
		foutexit << field.particles[j].position[2] << " ";
		foutexit << field.particles[j].velocity[0] << " ";
		foutexit << field.particles[j].velocity[1] << " ";
		foutexit << field.particles[j].velocity[2] << endl;
	}

	//CLEANUP
	fout.close();
	fin.close();
	foutexit.close();

	CloseHandle(hEventGo[0]);
	for (int t = 0; t < NUM_THREADS; t++) {
		CloseHandle(hEventDone[t]);
		CloseHandle(threads[t]);
	}
	for (int i = 0; i < field.particleCount; i++) {
		CloseHandle(particleMutexes[i]);
	}

	return 0;
}

void initBoundaryParticles(Field* field)
{

}

void initParticles(Field* field)
{
	srand(time(0));
	//ofstream fout ("BaselineRunTestPositions.txt");
	//initialize all the particles
	double initialBounds = field->initialBounds;
	for (int j = 0; j < field->particleCount; j++) {
		//give the particle a random position inside cube of side length initialBounds, centroid at the origin.
		field->particles[j].position[0] = initialBounds*(((double)rand())/((double)RAND_MAX) - 0.5);
		field->particles[j].position[1] = initialBounds*(((double)rand())/((double)RAND_MAX) - 0.5);
		field->particles[j].position[2] = initialBounds*(((double)rand())/((double)RAND_MAX) - 0.5);
		field->particles[j].tempPosition[0] = field->particles[j].position[0];
		field->particles[j].tempPosition[1] = field->particles[j].position[1];
		field->particles[j].tempPosition[2] = field->particles[j].position[2];

		//initialize the initial velocity, acceleration
		field->particles[j].velocity[0] = field->initVelocitySpread*2.0*(((double)rand())/((double)RAND_MAX) - 0.5);
		field->particles[j].velocity[1] = field->initVelocitySpread*2.0*(((double)rand())/((double)RAND_MAX) - 0.5);
		field->particles[j].velocity[2] = field->initVelocitySpread*2.0*(((double)rand())/((double)RAND_MAX) - 0.5);
		field->particles[j].acceleration[0] = 0;
		field->particles[j].acceleration[1] = 0;
		field->particles[j].acceleration[2] = 0;
		
		for (int i = 0; i < 4; i++) {
			field->particles[j].ka[i][0] = 0;
			field->particles[j].ka[i][1] = 0;
			field->particles[j].ka[i][2] = 0;

			field->particles[j].kv[i][0] = 0;
			field->particles[j].kv[i][1] = 0;
			field->particles[j].kv[i][2] = 0;
		}

		
		//gives the particles semi-random masses
		field->particles[j].mass = field->massAverage + field->massSpread*2.0*(((double)rand())/((double)RAND_MAX) - 0.5);

		field->particles[j].particleRadius = 2*pow((field->particles[j].mass/PIXEL_DENSITY)/MPI, 0.3333333333);
		field->particles[j].exists = true;
	}
}

void getCenterOfMass(Field* field)
{
	double fieldMass = 0;
	field->COM[0] = 0;
	field->COM[1] = 0;
	field->COM[2] = 0;

	for (int j = 0; j < field->particleCount; j++) {
		field->COM[0] += field->particles[j].mass * field->particles[j].position[0];
		field->COM[1] += field->particles[j].mass * field->particles[j].position[1];
		field->COM[2] += field->particles[j].mass * field->particles[j].position[2];
		fieldMass += field->particles[j].mass;
	}
	field->COM[0] /= fieldMass;
	field->COM[1] /= fieldMass;
	field->COM[2] /= fieldMass;
}

void incrementPosition(Field* field)
{
	//ofstream fout ("BaselineRunTestIncrement.txt");
	//NOTE: two loops are required because otherwise the particles that are dealt
	//with towards the end of the loop would be accelerated according to
	//the future positions of other particles.
	double localized = field->localized;
	//retrieve the new accelerations
	for (int k = 0; k < field->particleCount; k++) {
		if (field->particles[k].exists) {
			double betweenVector[3];
			double distance;
			double unitBetween[3];
			
			double accelOnk;
			double accelOni;
			//we don't need to calculate the forces with all other particles because we've already
			//calculated some, no need to do it twice. see below for details.
			for (int i = k+1; i < field->particleCount; i++) {
				//make sure that p2 hasn't been absorbed into another particle.
				if (field->particles[i].exists) {
					//find the vector from p1 to p2 and the unit vector in that direction
					betweenVector[0] = field->particles[i].position[0] - field->particles[k].position[0];
					betweenVector[1] = field->particles[i].position[1] - field->particles[k].position[1];
					betweenVector[2] = field->particles[i].position[2] - field->particles[k].position[2];
					distance = magnitude(betweenVector, 3);
					unitBetween[0] = betweenVector[0]/distance;
					unitBetween[1] = betweenVector[1]/distance;
					unitBetween[2] = betweenVector[2]/distance;
					
					//check to see if the two particles have collided
					if (distance <= field->particles[i].particleRadius + field->particles[k].particleRadius) {
						field->particles[i].exists = false;

						double totalMass = field->particles[i].mass + field->particles[k].mass;
						//choose a position between the particles based on their velocities and masses.
						double xDifference = field->particles[i].position[0] - field->particles[k].position[0];
						double yDifference = field->particles[i].position[1] - field->particles[k].position[1];
						double zDifference = field->particles[i].position[2] - field->particles[k].position[2];
						field->particles[k].position[0] += (xDifference)*(field->particles[i].mass/totalMass);
						field->particles[k].position[1] += (yDifference)*(field->particles[i].mass/totalMass);
						field->particles[k].position[2] += (zDifference)*(field->particles[i].mass/totalMass);
						
						//update particle velocity based on the other particle's velocity
						field->particles[k].velocity[0] = (field->particles[k].velocity[0]*field->particles[k].mass + field->particles[i].velocity[0]*field->particles[i].mass)/(totalMass);
						field->particles[k].velocity[1] = (field->particles[k].velocity[1]*field->particles[k].mass + field->particles[i].velocity[1]*field->particles[i].mass)/(totalMass);
						field->particles[k].velocity[2] = (field->particles[k].velocity[2]*field->particles[k].mass + field->particles[i].velocity[2]*field->particles[i].mass)/(totalMass);
						
						//combine the two masses
						field->particles[k].mass = totalMass;
						
						//recalculate the particle's particleRadius based on its new mass
						field->particles[i].particleRadius = 2*pow((field->particles[k].mass/PIXEL_DENSITY)/MPI, 0.3333333333);
					} else if (distance*localized <= (field->initialBounds)/6.0) {
					//the particles have not collided, and they are within the simulation radius,
					//so we calculate the acceleration on p1 due to 
					//the gravity between p1 and p2.
						double fieldpermass = GRAVITY/(distance*distance);
						accelOni = (field->particles[k].mass)*fieldpermass;
						accelOnk = (field->particles[i].mass)*fieldpermass;

						field->particles[k].acceleration[0] += unitBetween[0]*accelOnk;
						field->particles[k].acceleration[1] += unitBetween[1]*accelOnk;
						field->particles[k].acceleration[2] += unitBetween[2]*accelOnk;
						
						//since we don't want to calculate this all over again, we add it to p2 as well
						field->particles[i].acceleration[0] -= unitBetween[0]*accelOni;
						field->particles[i].acceleration[1] -= unitBetween[1]*accelOni;
						field->particles[i].acceleration[2] -= unitBetween[2]*accelOni;
					}
				}
			}
		}
	}
	//update the velocity and position of the particles
	double timeStep = field->timeStep;
	for (int j = 0; j < field->particleCount; j++) {
		if (field->particles[j].exists) {
			field->particles[j].position[0] += (field->particles[j].velocity[0])*timeStep + 0.5*(field->particles[j].acceleration[0])*timeStep*timeStep;
			field->particles[j].position[1] += (field->particles[j].velocity[1])*timeStep + 0.5*(field->particles[j].acceleration[1])*timeStep*timeStep;
			field->particles[j].position[2] += (field->particles[j].velocity[2])*timeStep + 0.5*(field->particles[j].acceleration[2])*timeStep*timeStep;
			
			field->particles[j].velocity[0] += (field->particles[j].acceleration[0])*timeStep;
			field->particles[j].velocity[1] += (field->particles[j].acceleration[1])*timeStep;
			field->particles[j].velocity[2] += (field->particles[j].acceleration[2])*timeStep;
			
			field->particles[j].acceleration[0] = 0;
			field->particles[j].acceleration[1] = 0;
			field->particles[j].acceleration[2] = 0;
		}
	}
}

void calculateRungeKuttaK(Field* field, int subStep)
{
	//ofstream fout ("RungeKutta.txt");
	double localized = field->localized;
	//find the ks
	for (int k = 0; k < field->particleCount; k++) {
		if (field->particles[k].exists) {
			double betweenVector[3];
			double distance;
			double unitBetween[3];
			
			double accelOnk;
			double accelOni;
			for (int i = k+1; i < field->particleCount; i++) {
				//make sure that p2 hasn't been absorbed into another particle.
				if (field->particles[i].exists) {
					//find the vector from p1 to p2 and the unit vector in that direction
					betweenVector[0] = field->particles[i].tempPosition[0] - field->particles[k].tempPosition[0];
					betweenVector[1] = field->particles[i].tempPosition[1] - field->particles[k].tempPosition[1];
					betweenVector[2] = field->particles[i].tempPosition[2] - field->particles[k].tempPosition[2];
					distance = magnitude(betweenVector, 3);
					unitBetween[0] = betweenVector[0]/distance;
					unitBetween[1] = betweenVector[1]/distance;
					unitBetween[2] = betweenVector[2]/distance;
					
					//check to see if the two particles have collided
					if (distance <= field->particles[i].particleRadius + field->particles[k].particleRadius) {
						field->particles[i].exists = false;

						double totalMass = field->particles[i].mass + field->particles[k].mass;
						//choose a position between the particles based on their velocities and masses.
						double xDifference = field->particles[i].tempPosition[0] - field->particles[k].tempPosition[0];
						double yDifference = field->particles[i].tempPosition[1] - field->particles[k].tempPosition[1];
						double zDifference = field->particles[i].tempPosition[2] - field->particles[k].tempPosition[2];
						field->particles[k].position[0] += (xDifference)*(field->particles[i].mass/totalMass);
						field->particles[k].position[1] += (yDifference)*(field->particles[i].mass/totalMass);
						field->particles[k].position[2] += (zDifference)*(field->particles[i].mass/totalMass);
						field->particles[k].tempPosition[0] = field->particles[k].position[0];
						field->particles[k].tempPosition[1] = field->particles[k].position[1];
						field->particles[k].tempPosition[2] = field->particles[k].position[2];
						
						//update particle velocity based on the other particle's velocity
						field->particles[k].velocity[0] = (field->particles[k].velocity[0]*field->particles[k].mass + field->particles[i].velocity[0]*field->particles[i].mass)/(totalMass);
						field->particles[k].velocity[1] = (field->particles[k].velocity[1]*field->particles[k].mass + field->particles[i].velocity[1]*field->particles[i].mass)/(totalMass);
						field->particles[k].velocity[2] = (field->particles[k].velocity[2]*field->particles[k].mass + field->particles[i].velocity[2]*field->particles[i].mass)/(totalMass);
						
						//combine the two masses
						field->particles[k].mass = totalMass;
						
						//recalculate the particle's particleRadius based on its new mass
						field->particles[i].particleRadius = 2*(int)(pow((field->particles[k].mass/PIXEL_DENSITY)/MPI, 0.3333333333));
					} else if (distance*localized <= (field->initialBounds)/6.0) {
					//the particles have not collided, and they are within the simulation radius,
					//so we calculate the acceleration on p1 due to 
					//the gravity between p1 and p2.
						double fieldpermass = GRAVITY/(distance*distance);
						accelOni = (field->particles[k].mass)*fieldpermass;
						accelOnk = (field->particles[i].mass)*fieldpermass;

						field->particles[k].ka[subStep][0] += unitBetween[0]*accelOnk;
						field->particles[k].ka[subStep][1] += unitBetween[1]*accelOnk;
						field->particles[k].ka[subStep][2] += unitBetween[2]*accelOnk;
						
						//since we don't want to calculate this all over again, we add it to p2 as well
						field->particles[i].ka[subStep][0] -= unitBetween[0]*accelOni;
						field->particles[i].ka[subStep][1] -= unitBetween[1]*accelOni;
						field->particles[i].ka[subStep][2] -= unitBetween[2]*accelOni;
					}
				} //i exists
			} //i for
		} //k exists
	} //k for
}

DWORD WINAPI calculateRungeKuttaK_MT(LPVOID lpParam)
{
	Holder* holder = (Holder*)lpParam;
	Field* field = holder->field;
	int subStep = 0;
	int startValue = holder->id;
	while (true) {
		//wait for the go-ahead signal
		WaitForSingleObject(hEventGo, 1000);

		//ofstream fout ("RungeKuttaMT.txt");
		double localized = field->localized;
		//find the ks
		for (int k = startValue; k < field->particleCount; k += NUM_THREADS) {
			if (field->particles[k].exists) {
				double betweenVector[3];
				double distance;
				double unitBetween[3];
				
				double accelOnk;
				double accelOni;
				for (int i = k+1; i < field->particleCount; i++) {
					//make sure that p2 hasn't been absorbed into another particle.
					if (field->particles[i].exists) {
						//find the vector from p1 to p2 and the unit vector in that direction
						betweenVector[0] = field->particles[i].tempPosition[0] - field->particles[k].tempPosition[0];
						betweenVector[1] = field->particles[i].tempPosition[1] - field->particles[k].tempPosition[1];
						betweenVector[2] = field->particles[i].tempPosition[2] - field->particles[k].tempPosition[2];
						distance = magnitude(betweenVector, 3);
						unitBetween[0] = betweenVector[0]/distance;
						unitBetween[1] = betweenVector[1]/distance;
						unitBetween[2] = betweenVector[2]/distance;
						
						//check to see if the two particles have collided
						if (distance <= field->particles[i].particleRadius + field->particles[k].particleRadius) {
							WaitForSingleObject(particleMutexes[i], 10);
							field->particles[i].exists = false;
							ReleaseMutex(particleMutexes[i]);
							
							WaitForSingleObject(particleMutexes[k], 10);
							double totalMass = field->particles[i].mass + field->particles[k].mass;
							//choose a position between the particles based on their velocities and masses.
							double xDifference = field->particles[i].tempPosition[0] - field->particles[k].tempPosition[0];
							double yDifference = field->particles[i].tempPosition[1] - field->particles[k].tempPosition[1];
							double zDifference = field->particles[i].tempPosition[2] - field->particles[k].tempPosition[2];
							field->particles[k].position[0] += (xDifference)*(field->particles[i].mass/totalMass);
							field->particles[k].position[1] += (yDifference)*(field->particles[i].mass/totalMass);
							field->particles[k].position[2] += (zDifference)*(field->particles[i].mass/totalMass);
							field->particles[k].tempPosition[0] = field->particles[k].position[0];
							field->particles[k].tempPosition[1] = field->particles[k].position[1];
							field->particles[k].tempPosition[2] = field->particles[k].position[2];
							
							//update particle velocity based on the other particle's velocity
							field->particles[k].velocity[0] = (field->particles[k].velocity[0]*field->particles[k].mass + field->particles[i].velocity[0]*field->particles[i].mass)/(totalMass);
							field->particles[k].velocity[1] = (field->particles[k].velocity[1]*field->particles[k].mass + field->particles[i].velocity[1]*field->particles[i].mass)/(totalMass);
							field->particles[k].velocity[2] = (field->particles[k].velocity[2]*field->particles[k].mass + field->particles[i].velocity[2]*field->particles[i].mass)/(totalMass);
							
							//combine the two masses
							field->particles[k].mass = totalMass;
							
							//recalculate the particle's particleRadius based on its new mass
							field->particles[k].particleRadius = 2*(int)(pow((field->particles[k].mass/PIXEL_DENSITY)/MPI, 0.3333333333));
							ReleaseMutex(particleMutexes[k]);
						} else if (distance*localized <= (field->initialBounds)/6.0) {
						//the particles have not collided, and they are within the simulation radius,
						//so we calculate the acceleration on p1 due to 
						//the gravity between p1 and p2.
							double fieldpermass = GRAVITY/(distance*distance);
							accelOni = (field->particles[k].mass)*fieldpermass;
							accelOnk = (field->particles[i].mass)*fieldpermass;

							WaitForSingleObject(particleMutexes[k], 10);
							field->particles[k].ka[subStep][0] += unitBetween[0]*accelOnk;
							field->particles[k].ka[subStep][1] += unitBetween[1]*accelOnk;
							field->particles[k].ka[subStep][2] += unitBetween[2]*accelOnk;
							ReleaseMutex(particleMutexes[k]);
							
							//since we don't want to calculate this all over again, we add it to p2 as well
							WaitForSingleObject(particleMutexes[i], 10);
							field->particles[i].ka[subStep][0] -= unitBetween[0]*accelOni;
							field->particles[i].ka[subStep][1] -= unitBetween[1]*accelOni;
							field->particles[i].ka[subStep][2] -= unitBetween[2]*accelOni;
							ReleaseMutex(particleMutexes[i]);
						}
					} //i exists
				} //i for
			} //k exists
		} //k for

		//let the main thread know that we're done with this k
		SetEvent(hEventDone[holder->id]);
		ResetEvent(hEventGo[0]);

		subStep++;
		subStep %= 4;
	}
	return 0;
}

void incrementTemporaryState(Field* field, int k)
{
	double timeStep = field->timeStep;
	if (k == 0 || k == 1) {
		timeStep /= 2.0;
	}
	for (int j = 0; j < field->particleCount; j++) {
		if (field->particles[j].exists) {
			//calculate the new kvs
			if (k == 0) {
				field->particles[j].kv[k][0] = field->particles[j].velocity[0];
				field->particles[j].kv[k][1] = field->particles[j].velocity[1];
				field->particles[j].kv[k][2] = field->particles[j].velocity[2];
			} else {
				field->particles[j].kv[k][0] = field->particles[j].velocity[0] + field->particles[j].ka[k-1][0]*timeStep;
				field->particles[j].kv[k][1] = field->particles[j].velocity[1] + field->particles[j].ka[k-1][1]*timeStep;
				field->particles[j].kv[k][2] = field->particles[j].velocity[2] + field->particles[j].ka[k-1][2]*timeStep;
			}
			//calculate the new positions
			if (k != 3) {
				field->particles[j].tempPosition[0] = field->particles[j].position[0] + (field->particles[j].kv[k][0])*timeStep;
				field->particles[j].tempPosition[1] = field->particles[j].position[1] + (field->particles[j].kv[k][1])*timeStep;
				field->particles[j].tempPosition[2] = field->particles[j].position[2] + (field->particles[j].kv[k][2])*timeStep;
			}
		}
	}
}

void incrementStateRungeKutta(Field* field) {
	double timeStep = field->timeStep;
	for (int j = 0; j < field->particleCount; j++) {
		for (int i = 0; i < 3; i++) {
			field->particles[j].position[i] += timeStep/6.0 *(field->particles[j].kv[0][i] + 2*(field->particles[j].kv[1][i]) + 2*(field->particles[j].kv[2][i]) + (field->particles[j].kv[3][i]));
			field->particles[j].tempPosition[i] = field->particles[j].position[i];
			field->particles[j].velocity[i] += timeStep/6.0 *(field->particles[j].ka[0][i] + 2*(field->particles[j].ka[1][i]) + 2*(field->particles[j].ka[2][i]) + (field->particles[j].ka[3][i]));
		}
	}
}

void resetRungeKuttaK(Field* field) {
	for (int j = 0; j < field->particleCount; j++) {
		for (int i = 0; i < 4; i++) {
			field->particles[j].ka[i][0] = 0;
			field->particles[j].ka[i][1] = 0;
			field->particles[j].ka[i][2] = 0;

			field->particles[j].kv[i][0] = 0;
			field->particles[j].kv[i][1] = 0;
			field->particles[j].kv[i][2] = 0;
		}
	}
}

double magnitude(double* vector, int dimensions)
{
	double s = 0;
	for (int i = 0; i < dimensions; i++) {
		s += vector[i]*vector[i];
	}
	return sqrt(s);
}


