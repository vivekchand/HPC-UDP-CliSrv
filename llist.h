struct entry {
	double price_;
	uint16_t size_;
	struct entry *link;
};
typedef struct entry NODE;

NODE* insert(NODE *head, uint16_t level, double price, uint16_t size)
{
	NODE *temp;
	temp = (NODE*)malloc(sizeof(NODE));
	
	if(temp == NULL)
	{
		printf("\nERROR: No Memory Left!!");
		exit(-1);
	}

	temp->price_ = price;
	temp->size_ = size;
	temp->link = NULL;

	if(head == NULL)
	{
		head = temp;		
		return head;
	}	

	if(head->link == NULL)
	{
		head->link = temp;
		return head;
	}

	int pos = level;
	NODE *cur,*next;
	cur = head;
	while(cur->link!=NULL && pos-->0)
		cur=cur->link;

	next = cur->link;
	cur->link = temp;
	temp->link = next;

	return head;
}

NODE* modify(NODE *head, uint16_t level, double price, uint16_t size)
{
	if(head == NULL)  // no entries!
		return NULL;

	NODE *cur;
	int pos = level;
	cur = head;
	while(cur->link!=NULL && pos-->0)
			cur = cur->link;

	if(pos>0){  // Invalid Position
		return head;
	}

	cur->price_ = price;
	cur->size_ = size;

	return head;
}

NODE* del(NODE* head,uint16_t level)
{
	NODE *cur,*prev,*next,*temp;
	cur = head;
	int pos = level;
	if(head == NULL)
		return head;

	if(pos==0)
	{
		temp = head;
		head = head->link;
		free(temp);
		return head;
	}

	while(cur!=NULL && pos-->0){
		prev = cur;
		cur = cur->link;
	}

	if(cur==NULL)
		return head;

	next = cur->link;	
	prev->link = next;

	free(cur);
	return head;
}

void display(NODE *head,FILE *fp)
{
	NODE *cur;
	cur = head;
	while(cur!=NULL)
	{
			fprintf(fp,"\n\t %d @ %.0lf",cur->size_,cur->price_);
			cur = cur->link;
	}
	fprintf(fp,"\n");
}


