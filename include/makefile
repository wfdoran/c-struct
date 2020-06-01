DEFAULT=-i4 -brs -br -bap -nut -l100

tree: tree.h
	indent $(DEFAULT) -T NODE -T TREE -T KEYVAL -T data_t tree.h -o tree_indent.h

array: array.h
	indent $(DEFAULT) -T TYPE -T data_t array.h -o array_indent.h

hash_table: hash_table.h
	indent $(DEFAULT) -T TYPE -T HNODE -T HTABLE -T HITER -T key_t -T value_t -T uint64_t hash_table.h -o hash_table_indent.h

linked_list: linked_list.h
	indent $(DEFAULT) -T LNODE -T LLIST -T data_t linked_list.h -o linked_list_indent.h

clean:
	rm -f *_indent.h
