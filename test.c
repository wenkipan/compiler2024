#include <stdio.h>
int main(){
  int a, b;
  scanf("%d%d", &a, &b);
  printf("%d %d %d\n", a % b, a / b, a - a / b * b);
  return 0;
}
