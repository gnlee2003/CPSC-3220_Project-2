#include <stdio.h>
#include <stdlib.h>

int main() {
    int *list = malloc(6 * sizeof(int));

    for(int i = 0; i < 6; i++)
        list[i] = i + 1;

    printf("Before realloc:\n");
    for(int i = 0; i < 6; i++)
        printf("%d ", list[i]);
    printf("\n");

    // Shrink to 5 integers
    int *temp = realloc(list, 5 * sizeof(int));
    if(temp == NULL) {
        free(list);
        return 1;
    }
    list = temp;

    printf("After realloc (5 ints):\n");
    for(int i = 0; i < 5; i++)
        printf("%d ", list[i]);
    printf("\n");

    free(list);
}