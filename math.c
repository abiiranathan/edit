#include <stdio.h>

int add(int a, int b) { return a + b; }

int main(void) {
  int x = 10;
  int y = 30;
  int c = add(x, y);

  printf("x + y = %d\n", c);
}
