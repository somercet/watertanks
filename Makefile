
# indent -linux -pcs -psl -brf -nfca -gts -nfc1 -gts
SCHEMAS += /usr/local/share/glib-2.0/schemas
CFLAGS += -std=gnu99 -pipe -Werror -Wall -g $$( pkgconf --cflags gtk+-3.0 )
LDFLAGS += $$( pkgconf --libs gtk+-3.0 )

all: /usr/local/share/glib-2.0/schemas/com.github.example.gschema.xml example

example: example.c xcchatview.c
	$(CC) $(CFLAGS) -o $@ $^	$(LDFLAGS)

$(SCHEMAS)/com.github.example.gschema.xml: com.github.example.gschema.xml
	cat $^ > $@

.PHONY: clean

clean:
	rm example



# -Wextra -fsanitize=undefined,address

# valgrind --tool=memcheck --suppressions=/usr/share/glib-2.0/valgrind/glib.supp
# --suppressions=/usr/share/gtk-3.0/valgrind/gtk.supp --leak-check=full --log-file=vgdump ./example
