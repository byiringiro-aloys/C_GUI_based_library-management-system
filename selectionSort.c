// selection sort here 

#include <stdio.h>


void swap(int *a,int *b) {
  int temp = *a;
  *a=*b;
  *b=temp;	
}



void select(int arr[],int length) {
	for(int i=0;i<length-1;i++) {
		int minIndex=i;
		for(int j=i+1;j<length;j++) {
			if(arr[minIndex]>arr[j]) {
			 minIndex = j;
			}
		}
		swap(&arr[i],&arr[minIndex]);
	}
}

int main(){
	int arr[]= {298,2,-4,24,54,-2,23,3234,2,10};
	int n = sizeof(arr)/sizeof(arr[0]);
	select(arr, n);
	printf("\n ========Sorted array========\n");
	for (int a=0; a<n; a++){
		printf("%d ", arr[a]);
	}
	return 0;
}

//#include <stdio.h>
//int main() {
//int n, reverse = 0, remainder;
//printf("Enter an integer: ");
//scanf("%d", &n);
//while (n != 0) {
//remainder = n % 10;
//reverse = reverse * 10 + remainder;
//n /= 10;
//}
//printf("Reversed number = %d", reverse);
//return 0;
//}