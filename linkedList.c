#include <stdio.h>
#include <stdlib.h>

struct Node {
    int data;
    struct Node *Next;
};

int main() {
    int n, value;
    printf("Enter number of nodes: ");
    scanf("%d", &n);

    struct Node *Head = NULL;
    struct Node *temp = NULL;
    struct Node *current = NULL;

    for (int i = 0; i < n; i++) {
        printf("Enter value for node %d: ", i + 1);
        scanf("%d", &value);

        // Create a new node
        temp = (struct Node*)malloc(sizeof(struct Node));
        temp->data = value;
        temp->Next = NULL;

        if (Head == NULL) {
            Head = temp;          // First node becomes Head
            current = Head;
        } else {
            current->Next = temp; // Link previous node to new one
            current = current->Next;
        }
    }

    // Print the linked list
    printf("Linked list: ");
    current = Head;
    while (current != NULL) {
        printf("%d\t -> ", current->data);
        printf("%d\t",current->Next);
        current = current->Next;
    }
    printf("NULL\n");
    return 0;
}
