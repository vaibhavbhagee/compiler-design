int printf(char* str, ...);

// int a[10];
// int global_b;

int main(int argc, char **argv)
{

	int a[10], size;

	size = 9;

	int i;

	for (i = size; i >= 0; i--)
	{
		a[size-i] = i;
	}

	for (i = 0; i < size; i++)
	{
		printf("%d ", a[i]);
	}

	printf("%s\n", "After selection sort!!");

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

	for (i = 0; i < size; i++)
	{
		printf("%d ", a[i]);
	}

	return 0;
}