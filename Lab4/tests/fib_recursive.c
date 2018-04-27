int printf(char* str, ... );

int fib(int n) {

	if (n == 0 || n == 1) {
		return 1;
	}
	else {
		return (fib(n-1) + fib(n-2));
	}

	return 0;
}

int main(int argc, char** argv)
{
	int a[11];

	int i;
	i = 0;
	while (i <= 10) {
		a[i] = fib(i);
		i++;
	}

	i = 0;

	do {
		printf("%d ", a[i]);
		i++;
	} while (i <= 10);
	
	return 0;
}