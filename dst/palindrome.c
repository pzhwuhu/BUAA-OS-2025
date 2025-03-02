#include <stdio.h>
int huiwen(int x){
	int num[7] = {0}, i=0;
	while(x) {
		num[i++] = x % 10;
		x /= 10;
	}
	for(int j=0;j<i;j++){
		if(num[j] != num[i-j-1])
			return 0;
	}
	return 1;
}

int main() {
	int n;
	scanf("%d", &n);

	if (huiwen(n)) {
		printf("Y\n");
	} else {
		printf("N\n");
	}
	return 0;
}
