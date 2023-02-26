CC      = gcc
DEBUG   = -ggdb
NOCRYPT =
OPT     = -O
PROF    =
WARN    = -Wall -Wuninitialized -Wmissing-prototypes -Wstrict-prototypes \
          -Wmissing-declarations -Wredundant-decls
CFLAGS	= $(WARN) $(OPT) $(PROF) $(DEBUG) $(NOCRYPT) -D_GNU_SOURCE
LFLAGS	= $(DEBUG) $(PROF)

LIBS    = -lwiringPi

BINARY	= 1802pi
O_FILES	= 1802pi.o

$(BINARY): $(O_FILES)
	$(CC) $(LFLAGS) $(LDFLAGS) -o $(BINARY) $(O_FILES) $(LIBS)

.c.o:
	$(CC) -c $(C_FLAGS) $< -o $@

clean:
	@rm -rf $(BINARY) $(O_FILES)

