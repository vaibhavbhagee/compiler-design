void printf(int **x, int y, int z);
int main(int a, int b);
void scanf(void);
float f1, **f2, f3;

int main(int a, int b)
{
  int a, b, c;
  int** d;
  printf(d, b, c);

  if (a == b) {
  	return a;
  }
  else if (a < b) {
  	return a;
  }
  else{
  	return b;
  }
}

void scanf(void){
	return;
}

void printf(int **a, int d, int c)
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

// int*** scanf(void) 
// {
// 	int i, j;
// 	j = 0;
// 	for (i = 0; i < 5; i++) {
// 		j = j + 2;
// 	}
// 	int*** k;
// 	***k = j;
// 	return k;
// }