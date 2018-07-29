#include <stdlib.h>
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

void dynarr_append(struct dynarr_t *ptr, void *add_me)
{
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

void dynarr_set(struct dynarr_t *ptr, int index, void *value)
{
}

void dynarr_free(struct dynarr_t *ptr)
{
	if (ptr) {
		if (ptr->data) {
			free(ptr->data);
		}
	}
}

