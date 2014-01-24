SRCDIR := src
OBJDIR := objs
DC := dmd -w -g -de -od$(OBJDIR) # llvm d compiler
DFLAGS := -c
SGENSRCDIR := src/sgen
INCLUDE := -Isrc

SOURCES=$(SGENSRCDIR)/waveforms.d 

OBJECTS=$(patsubst $(SGENSRCDIR)/%.d, $(OBJDIR)/%.o, $(SOURCES))

all: sgen

$(OBJDIR)/%.o : $(SGENSRCDIR)/%.d
	@mkdir -p $(@D)
	$(DC) $(DFLAGS) $<  	

sgen: $(OBJECTS) $(SRCDIR)/sgen.d
	$(DC) $(SRCDIR)/sgen.d $(INCLUDE) $(OBJECTS) 


clean:
	rm -fR objs 
	rm -f sgen
