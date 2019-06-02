
__kernel void quicksort(__global float* array,
						__global float* arrayFinal)
{
  
   
   unsigned int row = get_global_id(0); 
   int col = get_global_id(1);
   printf("row: %d, col: %d, array[5]: %d, array[col]: %d \n",row,col,array[5],array[col]);
   arrayFinal[row] = array[row];
}

// void swap(int *array , const int left, const int right)
// {
// 	int temp = 0;
// 	temp = array[left];
// 	array[left] = array[right];
// 	array[right] = temp;
// }

// __kernel void section(__global int* array,
// 						__global int final,
// 						int left,
// 						int right)
// {
  
//    //unsigned int row = get_global_id(0); 
//    //int col = get_global_id(1);

// 	// get a mid point in the array
// 	const int mid = left + (right - left) / 2;
// 	const int pivotPtr = array[mid];
// 	// move the mid point value to the front of the array.
// 	swap(array, mid, left);
// 	int i_left = (left + 1);
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
// 	swap(array[i_left - 1], array[left]);
// 	// return new mid point
	
// 	final = i_left - 1;
// }