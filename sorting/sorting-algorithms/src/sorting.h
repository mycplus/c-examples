/* sorting.h - seven sorting algorithms (eight functions) for int arrays, in C11 */
#ifndef SORTING_H
#define SORTING_H

#include <stddef.h>

void insertion_sort(int a[], size_t n);
void selection_sort(int a[], size_t n);
void bubble_sort(int a[], size_t n);
void shell_sort(int a[], size_t n);
int  merge_sort(int a[], size_t n);      /* returns -1 if malloc fails */
void heap_sort(int a[], size_t n);
void quick_sort(int a[], size_t n);      /* Hoare, middle pivot; heap_sort above INT_MAX */
void quick_sort_last(int a[], size_t n); /* Lomuto, last pivot; heap_sort above INT_MAX */

#endif
