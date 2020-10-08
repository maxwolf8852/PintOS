#include <stdio.h>
#include "lib/stdlib.h"
#include <syscall.h>


int plus(int* A, int now, int k, int N) {
	if (now == 0 && A[now + 1] - A[now] == 1) return 0;
	else if (A[now + 1] - A[now] == 1) return plus(A, now - 1, k, N);
	A[now]++;
	return 1;
}

int 
main(int argc, const char* argv[])  
{

  if(argc!=3) return EXIT_FAILURE;
  int k = atoi(argv[1]), N = atoi(argv[2]);
if(k==0 || N == 0 || k>N) return EXIT_FAILURE;
int A[k];
for(int i = 0; i<k; i++) A[i] =i+1;
	int now = k - 1;
	while (1) {
		printf("{");
		for (int i = 0; i < k-1; i++) printf("%d, ",A[i]);
		printf("%d",A[k-1]);
		printf("}");

		A[now]++;
		if (A[now] == N + 1) {
			A[now] = N;
			if (now == 0) break;
			if (plus(A, now - 1, k, N)==0) break;
			A[now] = A[now - 1] + 1;	
		}
		printf(", ");
	}


            

  return EXIT_SUCCESS;
}
