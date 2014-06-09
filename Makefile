SRCDIR := src
OBJDIR := objs
CC := gcc -Wall -std=gnu99 -g
CFLAGS := -c 
INCLUDE := -Isrc
LIBS := -lm

SOURCES=$(addprefix $(SRCDIR)/, waveforms.c string_allocator.c string_manip.c dynamic_wlist.c envelope.c WAV.c)

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
