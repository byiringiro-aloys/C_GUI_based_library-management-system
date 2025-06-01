#include <stdio.h>
#include <stdlib.h>

struct Node {
	int data;
	struct Node* next;
};

struct Node* head = NULL;

void insertNodeAtBeginning(int data) {
	struct Node* node1 = (struct Node*)malloc(sizeof(struct Node));
	node1->data = data;
	node1->next = head;
	head = node1;
}

void insertNodeAtIndex(int pos, int data) {
	if (pos < 0) {
		printf("Invalid position!\n");
		return;
	}
	if (pos == 0) {
		insertNodeAtBeginning(data);
		return;
	}

	struct Node* node = (struct Node*)malloc(sizeof(struct Node));
	node->data = data;
	struct Node* temp = head;
	for (int i = 0; i < pos - 1; i++) {
		if (temp == NULL) {
			printf("Position out of bound!\n");
			return;
		}
		temp = temp->next;
	}
	node->next = temp->next;
	temp->next = node;
}

void insertNodeAtEnd(int data) {
	struct Node* lastNode = (struct Node*)malloc(sizeof(struct Node));
	lastNode->data = data;
	lastNode->next = NULL;

	if (head == NULL) {
		head = lastNode;
		return;
	}

	struct Node* temp = head;
	while (temp->next != NULL) {
		temp = temp->next;
	}
	temp->next = lastNode;
}

struct Node* linearSearch(int key) {
	struct Node* temp = head;
	while (temp != NULL) {
		if (temp->data == key) {
			return temp;
		}
		temp = temp->next;
	}
	return NULL; // Important: if not found
}

int main() {
	insertNodeAtBeginning(10);
	insertNodeAtIndex(1, 30);
	insertNodeAtIndex(2, 40);
	insertNodeAtEnd(20);

	// Printing the linked list
	printf("Linked List:\n");
	struct Node* temp = head;
	while (temp != NULL) {
		printf("%d\n", temp->data);
		temp = temp->next;
	}

	// Performing Linear Search
	int key = 30;
	struct Node* foundNode = linearSearch(key);
	if (foundNode != NULL) {
		printf("Element %d found!\n", foundNode->data);
	} else {
		printf("Element %d not found.\n", key);
	}

	return 0;
}
