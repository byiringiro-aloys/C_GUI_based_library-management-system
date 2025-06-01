//#include <stdio.h>
//
//int main() {
//    char str[100];
//
//    printf("Enter a line: ");
//    if (fgets(str, sizeof(str), stdin)) {
//        printf("You entered: %s", str);
//    } else {
//        printf("Error reading input.\n");
//    }
//
//    return 0;
//}


#include <stdio.h>
#include <string.h>

int main() {
    char str1[] = "apple";
    char str2[] = "banana";

    int result = strcmp(str2, str1);

    if (result == 0)
        printf("Strings are equal.\n");
    else if (result < 0)
        printf("%s comes before %s\n", str1, str2);
    else
        printf("%s comes after %s\n", str1, str2);

    return 0;
}
