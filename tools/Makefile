CC = gcc 
CFLAGS = -I$(src)../include

OBJS = main haap

all: $(OBJS)

$(OBJS): %: %.c
	 $(CC) $(CFLAGS) -o $@ $<

clean: 
	rm -f $(wildcard $(OBJS)) 
