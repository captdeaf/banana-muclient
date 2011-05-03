# Makefile for BananaClient
#
# Muchas gracias to mongoose

PROG  = server
CFLAGS=	-W -Wall -I. -pthread -g -lc -ggdb -Werror -D_GNU_SOURCE
LDFLAGS = -lpthread -ldl -lc -ggdb -lrt -liconv

all: $(PROG)

include Makefile.depend

C_FILES = api.c banana.c conf.c events.c mongoose.c sessions.c util.c users.c api_world.c file.c worlds.c net.c genapi.c logger.c api_user.c api_file.c strutil.c api_log.c mudparser.c
# C_FILES = *.c
O_FILES = $(patsubst %.c, build/%.o, $(C_FILES))

$(PROG): $(O_FILES)
	$(CC) $(LDFLAGS) -o $(PROG) $(O_FILES)

build/%.o: %.c
	@mkdir -p build
	$(CC) -c $(CFLAGS) $< -o $(patsubst %.c,build/%.o,$<)

clean:
	rm $(O_FILES) $(PROG)

API_FILES = $(filter api_%.c,$(C_FILES))

genapi.h: $(API_FILES)
	@type -p pcregrep >/dev/null || (echo "pcregrep is not installed" && false)
	@echo "Generating genapi.h"
	@echo "/* AUTO-GENERATED FILE, DO NOT EDIT */" > genapi.h
	@echo >> genapi.h
	@echo '#ifndef __GENAPI_H_ ' >> genapi.h
	@echo '#define __GENAPI_H_ ' >> genapi.h
	@echo '#include "banana.h"' >> genapi.h
	@echo >> genapi.h
	@grep -h ^ACTION $(API_FILES) | sed 's/).*/);/' >> genapi.h
	@echo >> genapi.h
	@echo "extern ActionList allActions[];" >> genapi.h
	@echo >> genapi.h
	@echo '#endif' >> genapi.h

genapi.c: $(API_FILES)
	@type -p pcregrep >/dev/null || (echo "pcregrep is not installed" && false)
	@echo "Generating genapi.c"
	@echo "/* AUTO-GENERATED FILE, DO NOT EDIT */" > genapi.c
	@echo >> genapi.c
	@echo '#include "banana.h"' >> genapi.c
	@echo '#include "genapi.h"' >> genapi.c
	@echo >> genapi.c
	@echo "ActionList allActions[] = {" >> genapi.c
	@grep -h ^ACTION $(API_FILES) | pcregrep -o '".*\w' | sed s/$$/\\},/ | sed 's/^/  {/' >> genapi.c
	@echo "  {NULL, NULL, 0}" >> genapi.c
	@echo "};" >> genapi.c
	@echo >> genapi.c

depend:
	touch Makefile.depend.in
	makedepend -fMakefile.depend.in -pbuild/ -w10 -- $(CFLAGS) -- $(C_FILES) 2>/dev/null
	grep -v ": /" Makefile.depend.in > Makefile.depend
	rm Makefile.depend.in Makefile.depend.in.bak

vgrun:
	valgrind ./$(PROG)

guestuser:
	mkdir -p users/guest
	echo yes > users/guest/can_guest
	echo 1 > users/guest/guestcount
	echo -n 'webcat' > users/guest/client
	echo -n 'guest' | md5sum > users/guest/password.md5

iconv:
	wget http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.13.1.tar.gz
	tar xvzf libiconv-1.13.1.tar.gz
	cd libiconv-1.13.1 ; ./configure --prefix=/usr ; make ; sudo make install

run:
	./$(PROG)
