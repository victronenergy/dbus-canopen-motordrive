#include <stdio.h>

void taskEarlyInit(void)
{
    puts("Task early initialized");
}

void taskInit(void)
{
    puts("Task initialized");
}

void taskUpdate(void)
{
    puts("Task updated");
}

void taskTick(void)
{
    puts("Task ticked");
}

char const *pltProgramVersion(void)
{
	return "v0.0";
}