void printf(int x, int y, int z), scanf(void);
int a[4], b[10];
float f1, *f2, f2;

int main(void)
{
  int a, b, c;
  float aa1;
  aa1 = 2.0;
  printf(a, b, c);

  return a + b;
}

void printf(int a, int b, int c)
{
	c = 1;
	if (c+1 == 2 || c - 1 == 0)
	{
		c = 0;
	}
	
	if (!c)
	{
		c = 1;
	}
	else
	{
		c = 0;
	}

	while (!c)
	{
		c = 1;
	}

	do {
		;
	}
	while(!c);

	return;
}

void scanf(void) 
{
	int i, j;
	j = 0;
	for (i = 0; i < 5; i++) {
		j = j + 2;
	}
}