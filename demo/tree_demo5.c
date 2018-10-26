#include <stdio.h>

#define data_t char*
#define prefix str
#include <tree.h>
#undef prefix
#undef data_t

int main(void) {
    tree_str_t *t = tree_str_init();
    
	tree_str_insert(t, "Ann", "111-222-3333");
	tree_str_insert(t, "Bill", "123-456-7890");
	tree_str_insert(t, "Charlie", "999-999-9999");
	tree_str_insert(t, "Dave", "656-666-6666");
	
	key_str_value_t rv = tree_str_retrieve(t, "Bill");
	printf("%s\n", rv.value);
	
    tree_str_destroy(&t);
	return 0;
}