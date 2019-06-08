//#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable

// __kernel void quicksort(__global float* array,
// 						__global float* arrayFinal)
// {
  
   
//    unsigned int row = get_global_id(0); 
//    int col = get_global_id(1);
//    printf("row: %d, col: %d, array[5]: %d, array[col]: %d \n",row,col,array[5],array[col]);
//    arrayFinal[row] = array[row];
// }

// // __kernel void section(__global int* array,
// // 						__global int* arrayFinal,
// // 						__global int* left,
// // 						int right)
// // {
  
   
// //    unsigned int row = get_global_id(0); 
// //    int col = get_global_id(1);
// //    printf("row: %d, col: %d, array[row]: %d, array[col]: %d \n",row,col,array[row],array[col]);
// //    arrayFinal[row] = array[row];
// // }

// void swap( int *left, int* right)
// {
// 	int temp = 0;
// 	temp = left;
// 	left = right;
// 	right = temp;
// }

// __kernel void section(__global int* array,
// 						__global int* arrayFinal,
// 						__global int* left,
// 						int right)
// {
//   	//int left = left0[0];
//    unsigned int row = get_global_id(0); 
//    int col = get_global_id(1);
//    //printf("row:%d col:%d\n", row, col);

// 	// get a mid point in the array

// 	const int mid = left[0] + (right - left[0]) / 2;
// 	const int pivotPtr = array[mid];
// 	// move the mid point value to the front of the array.
// 	swap(mid, left[0]);
// 	int i_left = (left[0] + 1);
// 	int j_right = right;
// 	// loop through section between leftPtr and rightPtr to find pivots correct placing
// 	// while swapping  < and > values in array to pivot with each other 

// 	while (i_left <= j_right) {
// 		// find next element from left  that is more then pivotPtr
// 		/// NOTE: checking for i_left and j_right are still valid
// 		while (i_left <= j_right && array[i_left] <= pivotPtr) {
// 			i_left = i_left + 1;
// 		}
// 		// find next element for far right which is smaller then pivot
// 		while (i_left <= j_right && array[j_right] > pivotPtr) {
// 			j_right = j_right - 1;
// 		}
// 		// double check if the left ptr is < right ptr. then swap
// 		if (i_left < j_right) {
// 			swap(array[i_left], array[j_right]);
// 		}
// 	}
// 	// swap original left with 
// 	swap(array[(i_left - 1)], array[left]);
// 	// return new mid point
// 	//int finalLeft = i_left - 1;
// 	arrayFinal = array;
// 	left[0] = i_left - 1;
// 	//printf("new left: %d\n",left[0]);
// }


__kernel void sectionQS(__global uint* inital,
						__global uint* final,
						__global uint* midPoint,
						__global uint* lowerIndex,
						__global uint* higherIndex)
{
	// mpiexec -n 2 M3.T1P.exe

	__local uint* lower[0] = 0;
	__local uint* higher[0] = 0;
	__local uint* lowerIndex_local;
	__local uint* higherIndex_local;

 	int col = get_global_id(0); 
	printf("col: %d, ", col);
	uint index = 0;
	uint index_local = 0;
	
	uint val = inital[col];
	uint mid = midPoint[0];

	barrier(CLK_LOCAL_MEM_FENCE);
	if(val > inital[mid])
	{
		//index = atomic_inc(&higherIndex[0]);
		index_local = higher[atomic_inc(higherIndex_local[0])] = val;
	}
	else
	{
		//index = atomic_inc(&lowerIndex[0]);
		index_local = lower[atomic_inc(lowerIndex_local[0])] = val;
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	barrier(CLK_GLOBAL_MEM_FENCE);
	if(val > inital[mid])
	{
		index = atomic_inc(&higherIndex[0]);
		final[index] = higher[index_local];
	}
	else
	{
		index = atomic_inc(&lowerIndex[0]);
		final[index] = lower[index_local];
	}
	// barrier(CLK_GLOBAL_MEM_FENCE);
	// if(col == 0)
	// {
	// 	//lowerIndex[0] = 0;
	// 	//higherIndex[0] = 0;
	// 	//printf("cols: %d, midPoint: %d, lowerIndex: %d, higherIndex: %d\n", col, midPoint[0], lowerIndex[0], higherIndex[0]);
	// }
	// barrier(CLK_GLOBAL_MEM_FENCE);
	//final[col] = inital[col];

	
	// printf("midpoint: %d\n",midPoint[0]);

	//printf("[mid:%d]- val:%d, ", mid, val);
	if(val > inital[mid])
	{
		index = atomic_inc(&higherIndex[0]);
		//index++;
		//printf("index: %d", index);
	}
	else
	{
		index = atomic_inc(&lowerIndex[0]);
		//printf("index: %d", index);
	}

	barrier(CLK_GLOBAL_MEM_FENCE);
	if(col == 0)
	{
		//final[lowerIndex[0]+1] = midPoint[0];
		uint p = lowerIndex[0]+1;
		//atomic_xchg(&midPoint[0], p);
		printf("lowerIndex[0]: %d\n", lowerIndex[0]);
		printf("higherIndex[0]: %d\n", higherIndex[0]);
		printf("midpoint: %d\n",midPoint[0]);
	}
	barrier(CLK_GLOBAL_MEM_FENCE);

	if(inital[col] > inital[mid])
	{
		int ind = lowerIndex[0]+2+index;
		final[ind] = inital[col];
	}
	else
	{
		final[index] = inital[col];
	}
}