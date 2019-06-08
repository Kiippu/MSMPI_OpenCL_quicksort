#include "mpi.h"
#include <ostream>
#include <iostream>
#include "Timer.h"
#include <vector>
#include <ctime>
#include <algorithm>

#include <CL/cl.h>


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <Windows.h>

const int LOCAL_WORK_DIVISOR = 16;
const int ARRAY_SIZE = LOCAL_WORK_DIVISOR * LOCAL_WORK_DIVISOR;
const int MAX_VALUE = 1000;

MPI_Status status;

/// final array
unsigned arrayToSort[ARRAY_SIZE];
/// helper off set and distance arrays
std::vector<unsigned> offSetArray;
std::vector<unsigned> offSetDistantaceArray;


/// gets the biggest aarray sector to spilt
int getBiggestArray() 
{
	// gets distances
	offSetDistantaceArray.clear();
	for (size_t i = 0; i < offSetArray.size()-1; i++)
	{
		offSetDistantaceArray.push_back(offSetArray[i+1] - offSetArray[i]);
	}

	//get left index of biggest distance
	int leftBig = 0;
	unsigned disBig = 0;
	for (int i = 0; i < offSetDistantaceArray.size(); i++)
	{
		if (offSetDistantaceArray[i] > disBig)
		{
			disBig = offSetDistantaceArray[i];
			leftBig = i;
		}
	}
	// return biggets distance left index
	return leftBig;
}

/// print results of current array
void print(unsigned* array, int arraySize)
{
	for (size_t i = 0; i < arraySize; i++)
	{
		printf("%d, ", array[i]);
	}
}

/// checks if each element after itself is larger
void checkArray(int* array, int arraySize)
{
	printf("\n\n.\n..\n...\n------------------------------------------------------------\nError Checking Array..\n");
	int errCount = 0;
	for (int i = 0; i < arraySize; i++)
	{
		for (int k = i; k < arraySize; k++)
		{
			if (array[i] > array[k])
			{
				printf("index %d is > index %d (%d > %d)\n", i, k, array[i], array[k]);
				errCount++;
			}
		}
	}
	printf("Error Checking Complete!\nErrors total = %d out of %d elements\n------------------------------------------------------------\n...\n..\n.\n\n", errCount, arraySize);
}

unsigned section(unsigned* array, const unsigned left, const unsigned right) {
	// get a mid point in the array
	const unsigned mid = left + (right - left) / 2;
	const unsigned pivotPtr = array[mid];
	// move the mid point value to the front of the array.
	std::swap(array[mid], array[left]);
	std::shared_ptr<unsigned> i_left = std::make_shared<unsigned>(left + 1);
	std::shared_ptr<unsigned> j_right = std::make_shared<unsigned>(right);
	// loop through section between leftPtr and rightPtr to find pivots correct placing
	// while swapping  < and > values in array to pivot with each other 

	while (*i_left <= *j_right) {
		// find next element from left  that is more then pivotPtr
		/// NOTE: checking for i_left and j_right are still valid
		while (*i_left <= *j_right && array[*i_left] <= pivotPtr) {
			*i_left = *i_left + 1;
		}
		// find next element for far right which is smaller then pivot
		while (*i_left <= *j_right && array[*j_right] > pivotPtr) {
			*j_right = *j_right - 1;
		}
		// double check if the left ptr is < right ptr. then swap
		if (*i_left < *j_right) {
			std::swap(array[*i_left], array[*j_right]);
		}
	}
	// swap original left with 
	std::swap(array[*i_left - 1], array[left]);
	// return new mid point
	return *i_left - 1;
}

int sectionOpenCL(unsigned * initialArray, unsigned& arraySize)
{

	//Sleep(10);
	/// begin OpenCL code
	int errMsg;                         // error code
	///Open CL vars
	cl_command_queue commands;
	cl_program program;
	cl_device_id device_id;
	cl_context context;
	cl_kernel kernel;

	/// OpenCL device memory for matrix 0,1,2
	cl_mem initArray;
	cl_mem finalArray;
	cl_mem midPoint_mem;
	cl_mem indexHigh_mem;
	cl_mem indexLow_mem;

	///Allocate host memory for the final matrix that is returned back to master thread
	unsigned intit_mem_size = sizeof(unsigned) * arraySize;
	unsigned* intit_host_mem = (unsigned*)malloc(intit_mem_size);

	unsigned final_mem_size = sizeof(unsigned) * arraySize;
	unsigned* final_host_mem = (unsigned*)malloc(final_mem_size);

	unsigned mid_mem_size = sizeof(unsigned);
	unsigned* mid_host_mem = (unsigned*)malloc(mid_mem_size);

	unsigned high_mem_size = sizeof(unsigned);
	unsigned* high_host_mem = (unsigned*)malloc(high_mem_size);

	unsigned low_mem_size = sizeof(unsigned);
	unsigned* low_host_mem = (unsigned*)malloc(low_mem_size);

	cl_uint dev_cnt = 1;
	clGetPlatformIDs(0, 0, &dev_cnt);

	cl_platform_id platform_ids[100];
	clGetPlatformIDs(dev_cnt, platform_ids, NULL);

	/// Connectcompute device and check for error
	int gpu = 1;
	errMsg = clGetDeviceIDs(platform_ids[0], gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device_id, NULL);
	if (errMsg != CL_SUCCESS)
	{
		printf("Error -  Failed to create a device\n");
		exit(1);
	}

	/// Create a context 
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &errMsg);
	if (!context)
	{
		printf("Error -  Failed to create a context\n");
		exit(1);
	}

	/// Create a command with properties
	commands = clCreateCommandQueueWithProperties(context, device_id, 0, &errMsg);
	if (!commands)
	{
		printf("Error - Failed to create a command with properties\n");
		exit(1);
	}

	///Load the file which containing the kernel code, .cl file
	//char string[128];
	FILE *fp;
	// my lovely personal path - worked as reletive path did not...?
	char fileName[] = "D://University//Uni 2019//Programming Paradigms//assignments//M3.T2C//MSMPI_OpenCL_quicksort//M3.T1P//quicksort.cl";//"vector_add_kernel.cl";
	char *source_str;
	size_t source_size;

	fp = fopen(fileName, "r");
	if (!fp) {

		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(0x100000);
	source_size = fread(source_str, 1, 0x100000, fp);
	fclose(fp);

	/// Create Kernel Program from file
	program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &errMsg);
	if (errMsg != CL_SUCCESS) {
		printf("Error - Failed to create OpenCL program from file %d\n", (int)errMsg);
		exit(1);
	}

	/// Build the executable from the kernel file
	errMsg = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	if (errMsg != CL_SUCCESS)
	{
		size_t len;
		char buffer[2048];
		printf("Error - Failed to build executable! %d\n", errMsg);
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		exit(1);
	}

	/// Create the compute kernel in the program we wish to run
	kernel = clCreateKernel(program, "sectionQS", &errMsg);
	if (!kernel || errMsg != CL_SUCCESS)
	{
		printf("Error - Failed to create kernel!\n");
		exit(1);
	}

	for (size_t i = 0; i < arraySize; i++)
	{
		intit_host_mem[i] = arrayToSort[i];
		final_host_mem[i] = 0;
		printf("%d, ", intit_host_mem[i]);
	}

	mid_host_mem[0] = (unsigned)(0);//(arraySize / 2);
	high_host_mem[0] = (unsigned)(0);
	low_host_mem[0] = (unsigned)(0);

	/// make the matrices data in to device memory for our calculation
	initArray = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, intit_mem_size, intit_host_mem, &errMsg);
	finalArray = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, final_mem_size, final_host_mem, &errMsg);
	midPoint_mem = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, mid_mem_size, mid_host_mem, &errMsg);
	indexHigh_mem = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, high_mem_size, high_host_mem, &errMsg);
	indexLow_mem = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, low_mem_size, low_host_mem, &errMsg);//

	if (!initArray || !finalArray || !midPoint_mem || !indexHigh_mem || !indexLow_mem)
	{
		printf("Error - Failed to allocate buffer device memory!\n");
		exit(1);
	}

	/// alocate global and local work sizes used in kernel
	size_t localWorkSize[1], globalWorkSize[1];
	/// add parameters to kernel method arguments
	errMsg = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&initArray);
	errMsg |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&finalArray);
	errMsg |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&midPoint_mem);
	errMsg |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&indexHigh_mem);
	errMsg |= clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&indexLow_mem);

	/// checks argments are valid
	if (errMsg != CL_SUCCESS)
	{
		printf("Error - Failed to set kernel arguments! %d\n", errMsg); 
		char result[4096];
		size_t size;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(result), result, &size);
		printf("0 - %s\n", result);
		exit(1);
	}

	float  result = ((float)arraySize / (float)LOCAL_WORK_DIVISOR);
	//printf("Error - arraySize is not divible by LOCAL_WORK_DIVISOR!  %d / %d == %f\n", arraySize, LOCAL_WORK_DIVISOR, result);

	/// Check is dicvisor is divisible. if nott he matrixe multiplication will not be 100% processed
	if ((arraySize % LOCAL_WORK_DIVISOR) != 0)
	{
		float  result = ((float)arraySize / (float)LOCAL_WORK_DIVISOR);
		printf("Error - MAX_MATRIX_LENGTH is not divible by LOCAL_WORK_DIVISOR!  %d / %d == %f\n", ARRAY_SIZE, LOCAL_WORK_DIVISOR, result);
		exit(1);
	}

	localWorkSize[0] = LOCAL_WORK_DIVISOR;
	globalWorkSize[0] = arraySize;

	errMsg = clEnqueueNDRangeKernel(commands, kernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	if (errMsg != CL_SUCCESS)
	{
		printf("Error - Failed to execute kernel, %d\n", errMsg);
		exit(1);
	}

	///get result from device after processing
	errMsg = clEnqueueReadBuffer(commands, finalArray, CL_TRUE, 0, final_mem_size, final_host_mem, 0, NULL, NULL);
	/// check if result is valid
	if (errMsg != CL_SUCCESS)
	{
		printf("Error: Failed to read output final array! %d\n", errMsg);
		char result[4096];
		size_t size;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(result), result, &size);
		printf("0 - %s\n", result);
		exit(1);
	}
	///get result from device after processing
	errMsg = clEnqueueReadBuffer(commands, midPoint_mem, CL_TRUE, 0, mid_mem_size, mid_host_mem, 0, NULL, NULL);
	/// check if result is valid
	if (errMsg != CL_SUCCESS)
	{
		printf("Error: Failed to read output midpoint! %d\n", errMsg);
		char result[4096];
		size_t size;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(result), result, &size);
		printf("1 - %s\n", result);
		exit(1);
	}



	/// place OpenCL matric back into MPI matrix
	// this had me stumped for a long time, I had a quarter populated matrix for 3 days...
	// eventuall figured it out as rows and cols where flipped and removed matrixRow var from loop
	for (unsigned k = 0; k < (arraySize); k++)
	{
		initialArray[k] = intit_host_mem[k];
	}
	unsigned midPointReturn = mid_host_mem[0];

	/// clean up OpenCL
	free(intit_host_mem);
	free(final_host_mem);
	free(mid_host_mem);
	free(low_host_mem);
	free(high_host_mem);

	clReleaseMemObject(initArray);
	clReleaseMemObject(finalArray);
	clReleaseMemObject(midPoint_mem);
	clReleaseMemObject(indexHigh_mem);
	clReleaseMemObject(indexLow_mem);

	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);

	printf("\nmidPointReturn: %d\n", midPointReturn);

	return midPointReturn;
}

void quicksort(unsigned *array, const int left, const int right, unsigned arraySize)
{
	// check if left and right ad still valid values
	if (left >= right)
		return;
	// get the new midpoint
	//int midPtr = section(array, left, right);
	//printf("b4 section: \n");
	//print(array, arraySize);
	int midPtr = sectionOpenCL(array, arraySize);
	//print(array, arraySize);
	//printf("after section: \n");
	//printf("arraySize: %d", arraySize);
	//quicksort(array, left, midPtr - 1, arraySize);
	printf("arraySize: %d", arraySize);
	//quicksort(array, midPtr + 1, right, arraySize);
}

void masterThread(int& processorID, int& processorNum
	, int& totalProcessors, int& processorDestination, int& sourceID, int& arraySubSet, int& left, int& right) {

	srand(time(NULL));

	for (size_t i = 0; i < ARRAY_SIZE; i++)
	{
		arrayToSort[i] = ((rand() % MAX_VALUE) + 1);
	}
	int arraySize = ARRAY_SIZE;

	/// off set array first an dlast element
	offSetArray.push_back(0);
	offSetArray.push_back((arraySize - 1));

	/// fill in other sectors depends on num of processors
	// will create sectors = num totalProcessors-1 
	// will split up largest sector on each iteration
	for (size_t i = 1; i < totalProcessors; i++)
	{
		int leftStart = getBiggestArray();
		printf("-- sorting from index: %d - %d\n",leftStart,leftStart+1);
		offSetArray.push_back(section(arrayToSort, offSetArray[leftStart], offSetArray[leftStart +1]));
		std::sort(offSetArray.begin(), offSetArray.end());
	}
	// print num of elements in each sector
	for (int i = 0; i < offSetDistantaceArray.size(); i++)
	{
		printf("distance[%d] - %d, ", i, offSetDistantaceArray[i]);
		printf("\n");
	}


	printf("\nfirst sort - \n");
	//print(arrayToSort, ARRAY_SIZE);
	printf("\nfirst sort - \n");

	/// set timer
	Timer::getInstance().addStartTime(eTimeLogType::TT_QUICKSORT, "Quicksort");
	/// tracks left elemenet to state with in sort
	left = 0;
	/// tracks offset index - different to for loop processor index
	int index = 0;
	for (processorDestination = 1; processorDestination <= totalProcessors; processorDestination++)
	{
		// finding left and right index
		left = offSetArray[index];
		right = offSetArray[index+1];
		if (processorDestination == totalProcessors)
			right++;
		arraySubSet = right - left;
		/// send off data to other processors
		MPI_Send(&left, 1, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		MPI_Send(&arraySubSet, 1, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		MPI_Send(&right, 1, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		MPI_Send(&arrayToSort[left], arraySubSet, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		
		index++;
	}

	/// get all data back
	for (int i = 1; i <= totalProcessors; i++)
	{
		sourceID = i;
		MPI_Recv(&left, 1, MPI_INT, sourceID, 2, MPI_COMM_WORLD, &status);
		MPI_Recv(&arraySubSet, 1, MPI_INT, sourceID, 2, MPI_COMM_WORLD, &status);
		MPI_Recv(&arrayToSort[left], arraySubSet, MPI_INT, sourceID, 2, MPI_COMM_WORLD, &status);
	}

	/// finish timer for multiplication
	Timer::getInstance().addFinishTime(eTimeLogType::TT_QUICKSORT);

	/// print time taken
	Timer::getInstance().printFinalTimeSheet();
	/// print final array sorted
	printf("\nLast sort - \n");
	//print(arrayToSort, arraySize);
	/// check final array is sorted from <
	//checkArray(arrayToSort, arraySize);

};

void workerThread(int& processorID, int& processorNum
	, int& totalProcessors, int& processorDestination, int& sourceID, int& arraySubSet, int& left, int& right) {

	sourceID = 0;
	MPI_Recv(&left, 1, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);
	MPI_Recv(&arraySubSet, 1, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);
	MPI_Recv(&right, 1, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);
	MPI_Recv(&arrayToSort, arraySubSet, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);
	printf("--: BEGIN MPI PROCCESS %d :--\n", processorID);

	/// assigning array data for quicksort
	unsigned * array;
	array = (unsigned *)calloc(arraySubSet, sizeof(unsigned));
	for (size_t i = 0; i < arraySubSet; i++)
	{
		array[i] = arrayToSort[i];
	}
	int arraySize = arraySubSet;

	/// run quicksort on worker data
	quicksort(array, 0, arraySize - 1, arraySize);
	/// print worker array once sorted
	//printf("\n- Process %d sorted data -\n", processorID);
	//print(array, arraySize);

	/// assignonf new data back to old array
	for (size_t i = 0; i < arraySubSet; i++)
	{
		arrayToSort[i] = array[i];
	}

	/// sending matrix data back to the master thread
	MPI_Send(&left, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
	MPI_Send(&arraySubSet, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
	MPI_Send(&arrayToSort, arraySubSet, MPI_INT, 0, 2, MPI_COMM_WORLD);
	printf("\n--: FINISH MPI PROCCESS %d :--\n", processorID);
};


int main(int argc, char **argv)
{
	int processorNum;
	int processorID;
	int totalProcessors;
	int processorDestination;
	int sourceID;
	int matrixRows;
	//int rowOffset;
	int left, right;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &processorID);
	MPI_Comm_size(MPI_COMM_WORLD, &processorNum);

	totalProcessors = processorNum - 1;

	/// Master Process 
	// in charge of sending and setting arrayToSort data to processors
	if (processorID == 0) {
		masterThread(processorID, processorNum, totalProcessors, processorDestination, sourceID, matrixRows, left, right);
	}

	/// All processors but master thread
	if (processorID > 0) {
		workerThread(processorID, processorNum, totalProcessors, processorDestination, sourceID, matrixRows, left, right);
	}
	
	/// clean up MPI
	MPI_Finalize();
	return 0;
}

