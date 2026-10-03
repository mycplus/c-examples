/* selection_sort.h - selection sort and two variants for int arrays */
#ifndef SELECTION_SORT_H
#define SELECTION_SORT_H

#include <stddef.h>

void selection_sort(int a[], size_t n);
void stable_selection_sort(int a[], size_t n);
void double_selection_sort(int a[], size_t n);

#endif
