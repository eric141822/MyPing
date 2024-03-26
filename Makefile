COMPILERFLAGS = -Wall -Wextra -Wno-sign-compare 



OBJECTS = src/myping.c

.PHONY: all clean

all : myping.out \

myping.out: $(OBJECTS)
	$(CC) $(COMPILERFLAGS) $^ -o $@



clean :
	$(RM) myping.out