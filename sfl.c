#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <errno.h>
#define DIE(assertion, call_description)				\
	do {								\
			if (assertion) {					\
					fprintf(stderr, "(%s, %d): ",			\
									__FILE__, __LINE__);		\
					perror(call_description);			\
					exit(errno);					\
			}							\
	} while (0)

#define command_lenght 600

typedef struct node {
	void *address;
	struct node *next;
	struct node *prev;
} node;

typedef struct list {
	int data_size;
	int size;
	struct node *head;
	struct node *tail;
} list;

typedef struct SFL {
	list *fl;
	int number_lists;
} SFL;

typedef struct mem_used {
	int data_size;
	void *address;
	void *data;
	int size_write;
	void *address_write;
} mem_used;

typedef struct used_space {
	mem_used *x;
	int size;
} used_space;

used_space *create_used_space(used_space **m)
{
	(*m) = malloc(sizeof(struct used_space));
	DIE(!(*m), "malloc");
	(*m)->x = malloc(sizeof(struct mem_used));
	DIE(!(*m)->x, "malloc");
	(*m)->size = 0;
	return *m;
}

void add_nodes(list *my_list, void *start_addres)
{
	int size = my_list->size;

	for (int i = 0; i < size; ++i) {
		node *new = malloc(sizeof(struct node));
		DIE(!new, "malloc");
		new->next = new->prev = NULL;
		if (i == 0) {
			new->address = start_addres;
			my_list->head = new;
			my_list->tail = new;
		} else {
			new->address = my_list->head->address + i * my_list->data_size;
			new->prev = my_list->tail;
			my_list->tail->next = new;
			my_list->tail = new;
		}
	}
}

SFL *create_vector(int number_lists, void *start_heap, int bytes_per_list)
{
	SFL *sfl = malloc(sizeof(struct SFL));
	DIE(!sfl, "malloc");
	sfl->number_lists = number_lists;
	sfl->fl = malloc(number_lists * sizeof(struct list));
	DIE(!sfl->fl, "malloc");
	for (int i = 0; i < number_lists; ++i) {
		sfl->fl[i].data_size = pow(2, 3 + i);
		sfl->fl[i].size = bytes_per_list / sfl->fl[i].data_size;
		add_nodes(&sfl->fl[i], start_heap + i * bytes_per_list);
	}
	return sfl;
}

void resize_vector(SFL **sfl, void *address, int diff, int bytes)
{
	(*sfl)->number_lists++;
	(*sfl)->fl = realloc((*sfl)->fl, (*sfl)->number_lists *
						 sizeof(struct list));
	DIE(!(*sfl)->fl, "realloc");

	for (int i = 0; i < (*sfl)->number_lists - 1; ++i) {
		if ((*sfl)->fl[i].data_size > diff) {
			for (int k = (*sfl)->number_lists - 1; k >= i + 1; --k) {
				(*sfl)->fl[k] = (*sfl)->fl[k - 1];
			}
			(*sfl)->fl[i].data_size = diff;
			(*sfl)->fl[i].size = 1;

			node *new = malloc(sizeof(struct node));
			DIE(!new, "malloc");
			new->address = address + bytes;
			new->next = NULL;
			new->prev = NULL;
			(*sfl)->fl[i].head = new;
			(*sfl)->fl[i].tail = new;
			break;
		}
	}
}

void init_heap(char *command, SFL **sfl, int *tip, int *total_memory)
{
	int number_lists;
	void *start_heap;
	int bytes_per_list;

	char *token = strtok(command, " ");
	int cnt = 0;
	while (token) {
		cnt++;
		if (cnt == 2)
			sscanf(token, "%p", &start_heap);
		else if (cnt == 3)
			number_lists = atoi(token);
		else if (cnt == 4)
			bytes_per_list = atoi(token);
		else if (cnt == 5)
			*tip = atoi(token);
		token = strtok(NULL, " ");
	}
	*total_memory = number_lists * bytes_per_list;
	*sfl = create_vector(number_lists, start_heap, bytes_per_list);
}

void MALLOC(SFL **sfl, char *command, used_space **m, int *frag,
		    int *malloc_calls)
{
	int bytes;
	char *token = strtok(command, " ");
	int cnt = 0;
	while (token) {
		cnt++;
		if (cnt == 2)
			bytes = atoi(token);
		token = strtok(NULL, " ");
	}

	for (int i = 0; i < (*sfl)->number_lists; ++i) {
		if ((*sfl)->fl[i].data_size == bytes && (*sfl)->fl[i].size > 0) {
			(*malloc_calls)++;
			if ((*m)->size) {
				(*m)->x = realloc((*m)->x, ((*m)->size + 1) *
								  sizeof(struct mem_used));
				DIE(!(*m)->x, "realloc");
				(*m)->size++;
			} else
				(*m)->size++;
			(*m)->x[(*m)->size - 1].data_size = (*sfl)->fl[i].data_size;
			(*m)->x[(*m)->size - 1].address = (*sfl)->fl[i].head->address;
			(*m)->x[(*m)->size - 1].data =
			malloc((*m)->x[(*m)->size - 1].data_size);
			memset((*m)->x[(*m)->size - 1].data, 0,
				   (*m)->x[(*m)->size - 1].data_size);
			(*m)->x[(*m)->size - 1].size_write = 0;

			node *aux = (*sfl)->fl[i].head;
			(*sfl)->fl[i].head = (*sfl)->fl[i].head->next;
			free(aux);
			(*sfl)->fl[i].size--;
			return;

		} else if ((*sfl)->fl[i].data_size > bytes && (*sfl)->fl[i].size > 0) {
			(*malloc_calls)++;
			(*frag)++;

			int diff = (*sfl)->fl[i].data_size - bytes;
			void *start_address = (*sfl)->fl[i].head->address;

			if ((*m)->size) {
				(*m)->x = realloc((*m)->x, ((*m)->size + 1) *
								  sizeof(struct mem_used));
				DIE(!(*m)->x, "realloc");
				(*m)->size++;
			} else
				(*m)->size++;
			(*m)->x[(*m)->size - 1].data_size = (*sfl)->fl[i].data_size - diff;
			(*m)->x[(*m)->size - 1].address = (*sfl)->fl[i].head->address;
			(*m)->x[(*m)->size - 1].data =
			malloc((*m)->x[(*m)->size - 1].data_size);
			memset((*m)->x[(*m)->size - 1].data, 0,
				  (*m)->x[(*m)->size - 1].data_size);
			(*m)->x[(*m)->size - 1].size_write = 0;

			node *aux = (*sfl)->fl[i].head;
			(*sfl)->fl[i].head = (*sfl)->fl[i].head->next;
			free(aux);
			(*sfl)->fl[i].size--;

			int cnt = -1;
			for (int k = 0; k < i; ++k) {
				if ((*sfl)->fl[k].data_size == diff) {
					cnt = k;
				}
			}
			if (cnt != -1) {
				node *new = malloc(sizeof(struct node));
				DIE(!new, "malloc");
				new->address = start_address + bytes;
				new->next = new->prev = NULL;

				if ((*sfl)->fl[cnt].size == 0) {
					(*sfl)->fl[cnt].head = new;
					(*sfl)->fl[cnt].tail = new;
				} else {
					node *ptr = (*sfl)->fl[cnt].head;
					while (ptr && ptr->address < new->address)
						ptr = ptr->next;
					if (!ptr) {    // adaugam la final
						new->prev = (*sfl)->fl[cnt].tail;
						(*sfl)->fl[cnt].tail->next = new;
						(*sfl)->fl[cnt].tail = new;
					} else if (ptr == (*sfl)->fl[cnt].head) {
						new->next = (*sfl)->fl[cnt].head;
						(*sfl)->fl[cnt].head->prev = new;
						(*sfl)->fl[cnt].head = new;
					} else {
						new->next = ptr;
						new->prev = ptr->prev;
						ptr->prev = new;
						new->prev->next = new;
					}
				}
				(*sfl)->fl[cnt].size++;

			} else {
				resize_vector(sfl, start_address, diff, bytes);
			}
			return;
		}
	}
	printf("Out of memory\n");
}

void free_block(SFL **sfl, int data_size, void *address)
{
	for (int i = 0; i < (*sfl)->number_lists; ++i) {
		if ((*sfl)->fl[i].data_size == data_size) {
			node *new = malloc(sizeof(struct node));
			DIE(!new, "malloc");
			new->address = address;
			new->next = new->prev = NULL;

			if ((*sfl)->fl[i].size == 0) {
				(*sfl)->fl[i].head = new;
				(*sfl)->fl[i].tail = new;
			} else {
				node *ptr = (*sfl)->fl[i].head;
				while (ptr && ptr->address < new->address) {
					ptr = ptr->next;
				}
				if (ptr == (*sfl)->fl[i].head) {
					new->next = ptr;
					ptr->prev = new;
					(*sfl)->fl[i].head = new;
				} else if (!ptr) {
					new->prev = (*sfl)->fl[i].tail;
					(*sfl)->fl[i].tail->next = new;
					(*sfl)->fl[i].tail = new;
				} else {
					new->next = ptr;
					new->prev = ptr->prev;
					ptr->prev = new;
					new->prev->next = new;
				}
			}
			(*sfl)->fl[i].size++;
			return;
		}
	}

	(*sfl)->number_lists++;
	(*sfl)->fl = realloc((*sfl)->fl, (*sfl)->number_lists *
						 sizeof(struct list));
	DIE(!(*sfl)->fl, "realloc");
	for (int i = 0; i < (*sfl)->number_lists - 1; ++i) {
		if ((*sfl)->fl[i].data_size > data_size) {
			for (int k = (*sfl)->number_lists - 1; k >= i+1; --k) {
				(*sfl)->fl[k] = (*sfl)->fl[k - 1];
			}
			(*sfl)->fl[i].data_size = data_size;
			(*sfl)->fl[i].size = 1;
			node *new = malloc(sizeof(struct node));
			DIE(!new, "malloc");
			new->address = address;
			new->next = NULL;
			new->prev = NULL;
			(*sfl)->fl[i].head = new;
			(*sfl)->fl[i].tail = new;
			return;
		}
	}
	node *new = malloc(sizeof(struct node));
	DIE(!new, "malloc");
	// daca trebuie adaugat la finalul vectorului
	new->next = new->prev = NULL;
	new->address = address;
	(*sfl)->fl[(*sfl)->number_lists - 1].data_size = data_size;
	(*sfl)->fl[(*sfl)->number_lists - 1].size = 1;
	(*sfl)->fl[(*sfl)->number_lists - 1].head = new;
	(*sfl)->fl[(*sfl)->number_lists - 1].tail = new;
}

void FREE(char *command, int tip, used_space **m, SFL **sfl, int *free_calls)
{
	char *token = strtok(command, " ");
	void *address;
	int cnt = 0;
	while (token) {
		if (cnt == 1)
			sscanf(token, "%p", &address);
		cnt++;
		token = strtok(NULL, " ");
	}
	bool allocated = false;
	if (tip == 0) {
		for (int i = 0; i < (*m)->size; ++i) {
			if ((*m)->x[i].address == address) {
				(*free_calls)++;
				allocated = true;
				free_block(sfl, (*m)->x[i].data_size, address);
				free((*m)->x[i].data);
				if ((*m)->size > 1 && i != (*m)->size - 1) {
					for (int k = i; k < (*m)->size - 1; ++k)
						(*m)->x[k] = (*m)->x[k + 1];
					(*m)->size--;
					(*m)->x = realloc((*m)->x, (*m)->size *
									  sizeof(struct mem_used));
					DIE(!(*m)->x, "realloc");
				} else if (i == (*m)->size - 1 && (*m)->size != 1) {
					(*m)->size--;
					(*m)->x = realloc((*m)->x, (*m)->size *
									  sizeof(struct mem_used));
					DIE(!(*m)->x, "realloc");
				} else {
					(*m)->size = 0;
				}
			}
		}
	}
	if (!allocated)
		printf("Invalid free\n");
}

bool verify_address(used_space **m, void *address)
{
	// verific daca adresa introdusa exista in vreun bloc
	for (int i = 0; i < (*m)->size; ++i)
		if (address >= (*m)->x[i].address && address < (*m)->x[i].address +
		   (*m)->x[i].data_size)
			return true;
	return false;
}

int which_block(used_space **m, void *address)
{
	// caut blocul ce contine adresa introdusa
	for (int i = 0; i < (*m)->size; ++i)
		if (address >= (*m)->x[i].address && address < (*m)->x[i].address +
		   (*m)->x[i].data_size)
			return i;
	return 0;
}

bool verify_malloc(used_space **m, int bytes, void *address)
{
	for (int i = 0; i < (*m)->size; ++i) {
		if (address >= (*m)->x[i].address && address < (*m)->x[i].address
		    + (*m)->x[i].data_size) {
			int size = address - (*m)->x[i].address;
			int remain_size = (*m)->x[i].data_size - size;
			if (bytes <= remain_size)
				return true;
			else {
				void *new_address = address + remain_size;
				bytes -= remain_size;
				while (verify_address(m, new_address) && bytes > 0) {
					int cnt = which_block(m, new_address);
					bytes -= (*m)->x[cnt].data_size;
					new_address = (*m)->x[cnt].address + (*m)->x[cnt].
					data_size;
				}
				if (bytes <= 0)
					return true;
			}
		}
	}
	return false;
}

void write_block(used_space **m, int bytes, void *address, char *date,
				 int **verif)
{
	size_t l = strlen(date);
	if ((unsigned int)bytes > l)
		bytes = l;

	if (verify_malloc(m, bytes, address)) {
		for (int i = 0; i < (*m)->size; ++i) {
			if (address >= (*m)->x[i].address && address < (*m)->x[i].address
			    + (*m)->x[i].data_size){
				int size = address - (*m)->x[i].address;
				int remain_size = (*m)->x[i].data_size - size;
				if (bytes <= remain_size) {
					memmove((*m)->x[i].data + size, date, bytes);
					// marcam ca si zona ocupata de write
					if ((*m)->x[i].size_write == 0) {
						(*m)->x[i].address_write = address;
						(*m)->x[i].size_write = bytes;
					} else {
						// daca exista ceva scris, aleg maximul de size_write
						// cat si minimul adresei de inceput
						(*m)->x[i].address_write = ((*m)->x[i].address_write <
						address) ? (*m)->x[i].address_write : address;
						(*m)->x[i].size_write = (*m)->x[i].size_write + bytes;
					}
				} else {
					memmove((*m)->x[i].data + size, date, remain_size);
					if ((*m)->x[i].size_write == 0) {
						(*m)->x[i].address_write = address;
						(*m)->x[i].size_write = remain_size;
					} else {
						(*m)->x[i].address_write = ((*m)->x[i].address_write <
						address) ? (*m)->x[i].address_write : address;
						(*m)->x[i].size_write = (*m)->x[i].size_write +
						remain_size;
					}

					bytes -= remain_size;
					void *new_address = address + remain_size;
					int characters = remain_size;

					while (bytes > 0) {
						int cnt = which_block(m, new_address);
						if (bytes <= (*m)->x[cnt].data_size) {
							memmove((*m)->x[cnt].data, date + characters,
									bytes);
							(*m)->x[cnt].address_write = new_address;
							(*m)->x[cnt].size_write = (bytes > (*m)->x[cnt].
							size_write) ? bytes : (*m)->x[cnt].size_write;
						} else {
							memmove((*m)->x[cnt].data, date + characters,
								   (*m)->x[cnt].data_size);
							(*m)->x[cnt].address_write = new_address;
							(*m)->x[cnt].size_write = (*m)->x[cnt].data_size;
						}
						bytes -= (*m)->x[cnt].data_size;
						characters += (*m)->x[cnt].data_size;
						new_address += (*m)->x[cnt].data_size;
					}
					return;
				}
			}
		}
	}
	else {
		printf("Segmentation fault (core dumped)\n");
		*(*verif) = 1;
	}
}

void WRITE(char *command, used_space **m, int *verif)
{
	char *command2 = malloc(command_lenght);
	DIE(!command2, "malloc");
	strcpy(command2, command);
	char *token = strtok(command, " ");
	void *address;
	char *date = malloc(command_lenght);
	DIE(!date, "malloc");
	date[0] = '\0';
	char *aux = date;
	int bytes;
	int cnt = 0;
	int space;
	while (token) {
		cnt++;
		if (cnt == 2)
			sscanf(token, "%p", &address);
		else if (cnt == 3){
			space = 0;
		}
		space++;
		token = strtok(NULL, " ");
	}
	space = space - 1;
	char *token2 = strtok(command2, " ");
	cnt = 0;
	int ok = 0;
	while (token2) {
		cnt++;
		if (cnt >= 3 && cnt < 3 + space) {
			if (ok == 0) {
				strcpy(aux, token2);
				ok = 1;
			} else {
				strcat(date, token2);
			}
		}
		if (cnt == 3 + space)
			bytes = atoi(token2);
		strcat(aux, " ");
		token2 = strtok(NULL, " ");
	}
	aux = aux + 1; // elimin " de la inceput
	for (unsigned int i = strlen(aux) - 1; i > 0; --i)
		if (aux[i] == '"'){
			aux[i] = '\0'; // de la final
			break;
		}
	write_block(m, bytes, address, aux, &verif);
	free(command2);
	free(date);
}

void DESTROY_HEAP(SFL **sfl, used_space **m, char **command)
{
	node *next = NULL;
	for (int i = 0; i < (*sfl)->number_lists; ++i) {
		node *current = (*sfl)->fl[i].head;
		if ((*sfl)->fl[i].size > 0) {
			while (current) {
				next = current->next;
				free(current);
				current = next;
			}
		}
	}
	free(next);
	free((*sfl)->fl);
	free(*sfl);
	for (int i = 0; i < (*m)->size; ++i)
		free((*m)->x[i].data);
	free((*m)->x);
	free(*m);
	free(*command);
	exit(0);
}

bool verify_malloc_read(used_space **m, int bytes, void *address)
{
	for (int i = 0; i < (*m)->size; ++i) {
		if (address >= (*m)->x[i].address && address < (*m)->x[i].address +
		   (*m)->x[i].data_size) {
			int size = address - (*m)->x[i].address;
			void *new_address = (*m)->x[i].address + size;
			if (new_address < (*m)->x[i].address_write)
				return false;
			else if (new_address >= (*m)->x[i].address_write +
					(*m)->x[i].size_write)
				return false;
			else {  // incep citirea dintr o zona valida
				int size2 = new_address - (*m)->x[i].address_write;
				// cat de departe suntem de inecputul zonei unde am scris
				int remained_space = (*m)->x[i].size_write - size2;
				// cat mai avem din zona de write pe primul bloc
				if (bytes <= remained_space)
					return true;
				else if (bytes > remained_space && new_address + bytes <
						 address + (*m)->x[i].data_size)
					return false;
				else { // ne am asigurat valabilitatea primului bloc
					bytes -= remained_space;
					new_address = address + (*m)->x[i].data_size;
					while (verify_address(m, new_address) && bytes > 0) {
						int cnt = which_block(m, new_address);
						if (new_address < (*m)->x[cnt].address_write)
							return false;
						int size3 = (*m)->x[cnt].size_write;
						// stiu ca incep de la inceput de bloc
						if (size3 < bytes && bytes <= (*m)->x[cnt].data_size)
							return false;
						bytes -= (*m)->x[cnt].data_size;
						new_address = new_address + (*m)->x[cnt].data_size;
					}
					if (bytes <= 0)
						return true;
				}
			}
		}
	}
	return false;
}

void read(char *command, used_space *m, int *verif2)
{
	char *token = strtok(command, " ");
	int cnt = 0;
	int bytes = 0;
	void *address;
	while (token) {
		if (cnt == 1)
			sscanf(token, "%p", &address);
		else if (cnt == 2)
			bytes = atoi(token);
		cnt++;
		token = strtok(NULL, " ");
	}
	if (!verify_malloc_read(&m, bytes, address))
		*verif2 = 1;
	else{
		for (int i = 0; i < m->size; ++i) {
			if (address >= m->x[i].address && address < m->x[i].address +
				m->x[i].data_size) {
				int characters = address - m->x[i].address_write;
				int space = address - m->x[i].address;
				int remained = m->x[i].data_size - space;
				int cnt2 = i;
				while (bytes > 0){
					if (cnt2 == i) {
						if (bytes > remained)
							printf("%.*s", remained, (char *)(m->x[cnt2].data
								   + space + characters));
						else
							printf("%.*s", bytes, (char *)(m->x[cnt2].data +
								   space + characters));
						address += remained;
						cnt2 = which_block(&m, address);
						bytes -= remained;
					} else {
						if (bytes <= m->x[cnt2].data_size)
							printf("%.*s", bytes, (char *)(m->x[cnt2].data));
						else
							printf("%.*s", m->x[i].data_size,
								  (char *)(m->x[cnt2].data));
						bytes -= m->x[cnt2].data_size;
						address += m->x[cnt2].data_size;
						cnt2 = which_block(&m, address);
					}
				}
				printf("\n");
				break;
			}
		}
	}
}

void dump_memory(SFL *sfl, used_space *m, int total_memory, int malloc_calls,
				 int frag, int free_calls)
{
	printf("+++++DUMP+++++\n");
	printf("Total memory: %d bytes\n", total_memory);
	int allocated_mem = 0;
	for (int i = 0; i < m->size; ++i)
		allocated_mem += m->x[i].data_size;
	printf("Total allocated memory: %d bytes\n", allocated_mem);
	printf("Total free memory: %d bytes\n", total_memory - allocated_mem);
	int free_blocks = 0;
	for (int i = 0; i < sfl->number_lists; ++i)
		free_blocks += sfl->fl[i].size;
	printf("Free blocks: %d\n", free_blocks);
	printf("Number of allocated blocks: %d\n", m->size);
	printf("Number of malloc calls: %d\n", malloc_calls);
	printf("Number of fragmentations: %d\n", frag);
	printf("Number of free calls: %d\n", free_calls);
	for (int i = 0; i < sfl->number_lists; ++i) {
		if (sfl->fl[i].size > 0) {
			printf("Blocks with %d bytes - %d free block(s) : ",
				   sfl->fl[i].data_size, sfl->fl[i].size);
			node *ptr = sfl->fl[i].head;
			while (ptr->next) {
				printf("%p ", ptr->address);
				ptr = ptr->next;
			}
			printf("%p\n", ptr->address);
		}
	}
	if (m->size > 0) {
		printf("Allocated blocks : ");
		for (int i = 0; i < m->size - 1; ++i)
			for (int j = i + 1; j < m->size; ++j)
				if (m->x[j].address < m->x[i].address) {
					mem_used aux = m->x[j];
					m->x[j] = m->x[i];
					m->x[i] = aux;
				}
		for (int i = 0; i < m->size - 1; ++i)
			printf("(%p - %d) ", m->x[i].address, m->x[i].data_size);
		printf("(%p - %d)\n", m->x[m->size - 1].address, m->x[m->size - 1].
			   data_size);
	} else
		printf("Allocated blocks :\n");
	printf("-----DUMP-----\n");
}

int main(void)
{
	char *command = malloc(command_lenght * sizeof(*command));
	DIE(!command, "malloc");
	SFL *sfl;
	used_space *m;
	int tip = 0;
	int total_memory = 0;
	int malloc_calls = 0;
	int frag = 0;
	int free_calls = 0;
	int verif = 0;
	int verif2 = 0;

	while (fgets(command, command_lenght, stdin)) {
		if (strstr(command, "INIT_HEAP")){
			init_heap(command, &sfl, &tip, &total_memory);
			m = create_used_space(&m);
		} else if (strstr(command, "MALLOC")) {
			MALLOC(&sfl, command, &m, &frag, &malloc_calls);
		} else if (strstr(command, "FREE")) {
			FREE(command, tip, &m, &sfl, &free_calls);
		} else if (strstr(command, "WRITE")) {
			WRITE(command, &m, &verif);
			if (verif == 1) {
				dump_memory(sfl, m, total_memory, malloc_calls, frag,
						    free_calls);
				DESTROY_HEAP(&sfl, &m, &command);
				exit(0);
			}
		} else if (strstr(command, "DESTROY")) {
			DESTROY_HEAP(&sfl, &m, &command);
		} else if (strstr(command, "DUMP")) {
			dump_memory(sfl, m, total_memory, malloc_calls, frag, free_calls);
		} else if (strstr(command, "READ")) {
			read(command, m, &verif2);
			if (verif2 == 1) {
				printf("Segmentation fault (core dumped)\n");
				dump_memory(sfl, m, total_memory, malloc_calls, frag,
						    free_calls);
				DESTROY_HEAP(&sfl, &m, &command);
				exit(0);
			}
		} else
			printf("Invalid command\n");
	}
	return 0;
}
