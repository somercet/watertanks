
# indent -linux -pcs -psl -brf -nfca -gts -nfc1 -gts
#CFLAGS += -DGLIB_DISABLE_DEPRECATION_WARNINGS -std=gnu99 -pipe -Werror -Wall -g $$( pkgconf --cflags gtk+-2.0 )
#LDFLAGS += $$( pkgconf --libs gtk+-2.0 )
CFLAGS += -DUSE_GTK3 -std=gnu99 -pipe -Werror -Wall -g $$( pkgconf --cflags gtk+-3.0 )
LDFLAGS += $$( pkgconf --libs gtk+-3.0 )

all: example

example: example.c xcchatview.c
	$(CC) $(CFLAGS) -o $@ $^	$(LDFLAGS)

.PHONY: clean

clean:
	rm example



# -Wextra -fsanitize=undefined,address

# valgrind --tool=memcheck --suppressions=/usr/share/glib-2.0/valgrind/glib.supp
# --suppressions=/usr/share/gtk-3.0/valgrind/gtk.supp --leak-check=full --log-file=vgdump ./example
