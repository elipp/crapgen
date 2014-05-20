SRCDIR := src
OBJDIR := objs
CC := gcc -Wall -std=c11
CFLAGS := -c -g
INCLUDE := -Isrc
LIBS := -lm

SOURCES=$(SRCDIR)/waveforms.c $(SRCDIR)/string_allocator.c

OBJECTS=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

all: sgen

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@

sgen: $(OBJECTS) $(SRCDIR)/sgen.c $(SRCDIR)/waveforms.c
	$(CC) $(SRCDIR)/sgen.c $(INCLUDE) $(OBJECTS) -o sgen $(LIBS)


clean:
	rm -fR objs 
	rm -f sgen
