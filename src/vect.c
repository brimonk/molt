#include <stdlib.h>
#include <string.h>
#include "vect.h"

void dynarr_init(struct dynarr_t *ptr, int obj_size)
{
	/* returns void so the user inspects the value of ptr->data */
	if (ptr) {
		ptr->data = malloc(obj_size * DYNARR_INITIAL_CAPACITY);

		if (ptr->data) {
			ptr->obj_size = obj_size;
			ptr->max_size = DYNARR_INITIAL_CAPACITY;
			ptr->curr_size = 0;
		}
	}
}

void dynarr_append(struct dynarr_t *ptr, void *add_me, int size)
{
	/* append an element to the dynarr, check if it'll be full first */
	struct dynarr_t *tmp;
	if (ptr->max_size <= ptr->curr_size) {
		tmp = realloc(ptr->data, (ptr->max_size * 2) * ptr->obj_size);

		if (tmp) {
			ptr->data = tmp;
			ptr->max_size *= 2;
		}
	}

	/* actually append the item, so get the pointer */
	tmp = DyNARR_GetPtr(ptr, ptr->curr_size); // (((char *)ptr->data) + (index * ptr->obj_size));

	/* now memcpy the data to the slot in the dynarr */
	memcpy(tmp, add_me, size);
	ptr->curr_size++;
}

void dynarr_setmaxsize(struct dynarr_t *ptr, int size)
{
	struct dynarr_t *tmp;
	if (ptr->max_size < size) {
		tmp = realloc(ptr->data, size * ptr->obj_size);

		if (tmp) {
			ptr->data = tmp;
			ptr->max_size = size;
		}
	}
}

void dynarr_setobjsize(struct dynarr_t *ptr, int size)
{
	/* should be a macro */
	ptr->obj_size = size;
}

void *dynarr_get(struct dynarr_t *ptr, int index)
{
	int byte_index;
	void *val;

	val = NULL;
	if (ptr) {
		byte_index = ptr->obj_size * index;

		val = (void *)(((char *)(ptr->data)) + byte_index);
	}

	return val;
}

void dynarr_set(struct dynarr_t *ptr, int index, void *value, int size)
{
	struct dynarr_t *tmp;

	tmp = DyNARR_GetPtr(ptr, index); // (((char *)ptr->data) + (index * ptr->obj_size));

	/* memcpy the data */
	memcpy(tmp, value, size);
}

void dynarr_free(struct dynarr_t *ptr)
{
	if (ptr) {
		if (ptr->data) {
			free(ptr->data);
		}
	}
}

