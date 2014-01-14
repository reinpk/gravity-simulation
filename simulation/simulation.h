/* GravitySimulation  --- simulation.h
 *
 *Peter K. Reinhardt - 2008
 *This code may not be distributed without the authors explicit written permission.
 */

const double GRAVITY = 0.00006674;
const double MPI = 1;
const double PIXEL_DENSITY = 1000;
const int MAX_PARTICLES = 3000;

//options
const bool MULTITHREADED = true;
const int NUM_THREADS = 2;
const bool EULER = false;
const bool VERBOSE = false;

struct Particle {
	double ka[4][3];
	double kv[4][3];

	double position[3];
	double tempPosition[3];
	double velocity[3];
	double acceleration[3];

	double mass;
	double particleRadius;
	bool exists;
};

struct Field {
	Particle particles[MAX_PARTICLES];

	int particleCount;
	double initialBounds;
	
	double initVelocitySpread;

	double massSpread;
	double massAverage;

	double COM[3];

	double timeStep;

	//options
	bool localized;
	bool boundarysmoothing;
};

struct Holder {
	Field* field;
	//id gives this thread's id, which is used to decide starting position in the particle array
	int id;
};

//multithreading stuff
HANDLE threads[NUM_THREADS];
DWORD dwThreadId[NUM_THREADS];
HANDLE particleMutexes[3000];
HANDLE hEventGo[1];
HANDLE hEventDone[NUM_THREADS];
Holder holders[NUM_THREADS];


//functions
void initBoundaryParticles(Field* field);
void initParticles(Field* field);
void getCenterOfMass(Field* field);
void incrementPosition(Field* field);
void calculateRungeKuttaK(Field* field, int subStep);
DWORD WINAPI calculateRungeKuttaK_MT(LPVOID lpParam);
void incrementTemporaryState(Field* field, int k);
void incrementStateRungeKutta(Field* field);
void resetRungeKuttaK(Field* field);
double magnitude(double* vector, int dimensions);
