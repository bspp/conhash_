cc = gcc
target = conhash
obj = util_rbtree.o md5.o conhash.o 
$(target):$(obj)
	$(cc) $(obj) -Wall -o $(target)
util_rbtree.o:util_rbtree.c
	$(cc) -c util_rbtree.c
md5.o:md5.c
	$(cc) -c md5.c
conhash.o:conhash.c
	$(cc) -c conhash.c
clean: 
	rm -rf $(obj)
	rm -rf $(target)
