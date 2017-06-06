CC	=	g++ -Wall
OPENCV =  `pkg-config --libs opencv` 
BINDIR = bin
OBJDIR = .obj
SRCDIR = src
SOURCES_C := $(wildcard $(SRCDIR)/*.c)
SOURCES_CPP  := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS  := $(SOURCES_CPP:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
EXECUTABLES := $(SOURCES_C:$(SRCDIR)/%.c=$(BINDIR)/%)
LIBS=${OPENCV}

all: $(EXECUTABLES)

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp 
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS) $(INCS)

$(EXECUTABLES): $(BINDIR)/% : $(SRCDIR)/%.c $(OBJECTS) 
	$(CC) -o $@ $< $(OBJECTS) $(LIBS) $(INCS)

.PHONY: all clear

clean:
	rm $(EXECUTABLES) 
	rm $(OBJECTS)


