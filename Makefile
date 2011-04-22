# Makefile for BananaClient
#
# Muchas gracias to mongoose

PROG  = server
CFLAGS=	-W -Wall -I. -pthread -g -lc
LDFLAGS = -lpthread -ldl -lc

all: $(PROG)

include Makefile.depend

C_FILES = api.c banana.c conf.c mongoose.c sessions.c util.c users.c api_world.c file.c
# C_FILES = *.c
O_FILES = $(patsubst %.c, build/%.o, $(C_FILES))

$(PROG): $(O_FILES)
	$(CC) $(LDFLAGS) -o $(PROG) $(O_FILES)

build/%.o: %.c
	@mkdir -p build
	$(CC) -c $(CCFLAGS) $< -o $(patsubst %.c,build/%.o,$<)

clean:
	rm $(O_FILES) $(PROG)

API_FILES = $(filter api_%.c,$(C_FILES))
api_inc.h: $(API_FILES)
	@echo "Generating api_inc.h"
	echo "/* AUTO-GENERATED FILE, DO NOT EDIT */" > api_inc.h
	echo >> api_inc.h
	echo "#ifndef _API_INC_H_" >> api_inc.h
	echo "#define _API_INC_H_" >> api_inc.h
	echo >> api_inc.h
	grep ^ACTION $(API_FILES) | sed 's/).*/);/' >> api_inc.h
	echo >> api_inc.h
	echo "ActionList allActions[] = {" >> api_inc.h
	grep ^ACTION $(API_FILES) | pcregrep -o '".*\w' | sed s/$$/\\},/ | sed 's/^/  {/' >> api_inc.h
	echo "  {NULL, NULL}" >> api_inc.h
	echo "};" >> api_inc.h
	echo >> api_inc.h
	echo "#endif /* _API_INC_H_ */" >> api_inc.h

depend:
	touch Makefile.depend.in
	makedepend -fMakefile.depend.in -pbuild/ -w10 -- $(CFLAGS) -- $(C_FILES) 2>/dev/null
	grep -v ": /" Makefile.depend.in > Makefile.depend
	rm Makefile.depend.in Makefile.depend.in.bak

run:
	./$(PROG)
