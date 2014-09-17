
PROGRAM  = twirlies

CSOURCES = glut-main.c twirlies.c
COBJECTS = $(CSOURCES:.c=.o)
CFLAGS   = $(COPT) $(CDEFS) $(CINCS)
CINCS    = 
CDEFS    = -DUNIX
COPT     = -g

LDFLAGS  = $(LDDIRS) $(LDLIBS)
LDDIRS   = 
LDLIBS   = -lglut -lGL -lGLU

SCRPROGRAM   = twirlies.scr
SCRCSOURCES  = scrsave.c twirlies.c
SCRCOBJECTS  = $(SCRCSOURCES:.c=.obj)
SCRCFLAGS    = /DWIN32 /D_MBCS /DNDEBUG /nologo
SCRLDFLAGS   = user32.lib gdi32.lib advapi32.lib scrnsave.lib opengl32.lib glu32.lib /link /subsystem:console
SCRCC        = cl.exe

all: $(PROGRAM)

$(PROGRAM): $(COBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(PROGRAM)
	$(RM) $(COBJECTS)

run: $(PROGRAM)
	./$<

