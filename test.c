




#include <stdio.h>




int main() {
	int a = 32;
	char buf[6] = {'0', '0', '0', '3', '2', '0'};

	char b = (char)a;
	printf("%d\n", b);

	printf("buf:%d\n", atoi(buf));
}



