#include<stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SIZE 4096*4096


void Fun()
{

  static int i = 1;
  printf("第%d次\n",i++);
}
int main()
{
  Fun();
  Fun();
  Fun();
  Fun();
  Fun();
  Fun();
  Fun();
  Fun();
  return 0;
}
