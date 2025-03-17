
#  xchat style: indent -l94 -ts3 -bli0 -i3 -ce
#CFLAGS += -DGLIB_DISABLE_DEPRECATION_WARNINGS

CFLAGS += -std=gnu99 -pipe -Werror -Wall -g $$( pkgconf --cflags gtk+-3.0 )
LDFLAGS += $$( pkgconf --libs gtk+-3.0 )

all: example

example: example.c xcchatview.c
	$(CC) $(CFLAGS) -o $@ $^	$(LDFLAGS)

newmodel: newmodel.c xcchatview.c
	$(CC) $(CFLAGS) -o $@ $^	$(LDFLAGS)

.PHONY: clean

clean:
	rm example newmodel

# -Wextra -fsanitize=undefined,address

# valgrind --tool=memcheck --suppressions=/usr/share/glib-2.0/valgrind/glib.supp
# --suppressions=/usr/share/gtk-3.0/valgrind/gtk.supp --leak-check=full --log-file=vgdump ./example
