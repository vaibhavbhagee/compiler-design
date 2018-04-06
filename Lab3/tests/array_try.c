int printf(char* str, ... );
int puts(char* str);

int printPtr(int* a, int len) {

	int i;

	puts("Print using pointer dereference: ");
	for (i = 0; i < len; i++)
	{
		printf("%d ", *(a+i));
	}
	puts("");

	return 0;
}

int printArr(int* a, int len) {

	int i, *c;
	c = a;

	puts("Print using array dereference: ");
	for (i = 0; i < len; i++)
	{
		printf("%d ", c[i]);
	}
	puts("");

	return 0;
}

int main(int argc, char** argv)
{
	int a[10];

	int i;
	for (i = 0; i<10; i++) {
		a[i] = i;
	}

	int res;
	puts("Send using array param: ");
	res = printArr(a,10);
	res = printPtr(a,10);

	puts("Send using ref first elt: ");
	res = printArr(&a[0],10);
	res = printPtr(&a[0],10);

	return 0;
}