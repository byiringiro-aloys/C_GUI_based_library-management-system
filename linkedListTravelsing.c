#include <stdio.h>
#include <stdlib.h>

struct Node{
	int data;
	struct Node* next;
};

void printLinkedList(struct Node* head){
	int count=0;
	struct Node* temp = head;
	while(temp!=NULL){
		printf("%d \n",temp->data);
		count++;
		temp = temp->next;
	};
	printf("Nodes visited are : %d",count);	
}

int countNodeRecursively(struct Node* head){
	if(head==NULL){
		return 0;
	}
	return 1 + countNodeRecursively(head->next);
}

int main(){
	struct Node* node1 = (struct Node*)malloc(sizeof(struct Node));
	struct Node* node2 = (struct Node*)malloc(sizeof(struct Node));
	struct Node* node3 = (struct Node*)malloc(sizeof(struct Node));
	]
	node1->data=10;
	node2->data=20;
	node3->data=30;
	
	node1->next=node2;
	node2->next=node3;
	node3->next=NULL;
	
	printLinkedList(node1);
	printf("\nThe recursive approach to visit each node: %d",countNodeRecursively(node1));
	
	free(node1);
	free(node2);
	free(node3);
	
	return 0;
}