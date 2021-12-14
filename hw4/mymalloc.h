# ifndef MYMALLOC_H
# define MYMALLOC_H

void myinit(int allocAlg);
void* mymalloc(size_t size);
void myfree(void* ptr);
void* myrealloc(void* ptr, size_t size);
void mycleanup();
double utilization();

# endif

