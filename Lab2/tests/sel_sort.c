int printf(char* str, ...);
int puts(char* str);

// int a[10];
// int global_b;

int printArray(int* c, int size) {
	int i, *a;
	a = c;
	puts("Printing array:");
	
	for (i = 0; i < size; ++i)
	{
		printf("%d ", a[i]);
	}
	puts("");

	return 0;
}

int fillArray(int* c, int size)
{
	int i, *a;
	a = c;
	// int a[10];

	for (i = size-1; i >= 0; i--)
	{
		a[size-1-i] = i;
	}

	return 0;
}

int selSort(int* c, int size)
{
	int i, *a;
	a = c;
	// int a[10];

	for (i = 0; i<size; i++)
	{
		int j;
		int min, min_index;
		min = a[i];
		min_index = i;
		for (j = i+1; j < size; j++)
		{
			if (a[j] < min)
			{
				min = a[j];
				min_index = j;
			}
		}

		int temp;
		temp = a[min_index];
		a[min_index] = a[i];
		a[i] = temp;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int a[10];
	int filled, sorted, printRes;

	filled = fillArray(a, 10);
	printRes = printArray(&a[0], 10);
	sorted = selSort(a, 10);	
	printRes = printArray(&a[0], 10);

	return 0;
}