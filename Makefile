SRCDIR := src
OBJDIR := objs
CC := clang -Wall -g
CFLAGS := -c 
INCLUDE := -Isrc
LIBS := -lm -lrt -lfftw3f

SOURCES=$(addprefix $(SRCDIR)/, waveforms.c string_allocator.c string_manip.c dynamic_wlist.c envelope.c utils.c track.c timer.c WAV.c)

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
