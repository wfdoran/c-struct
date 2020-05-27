DEFAULT=-i4 -brs -br -bap -nut -l100

tree: tree.h
	indent $(DEFAULT) -T NODE -T TREE -T KEYVAL -T data_t tree.h -o tree_indent.h

array: array.h
	indent $(DEFAULT) -T TYPE -T data_t array.h -o array_indent.h

