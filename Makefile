cc = gcc
ar = ar
ld = ld
rm = rm

cflags = -fPIC
ldflags= -shared
src = cparse_core.c XConfig.c
objs = $(src:.c=.o)

static_output = libXConfig.a
shared_output = libXConfig.so
all_outputs = $(static_output) $(shared_output)

all: $(all_outputs)

$(static_output): $(objs)
	$(ar) rcs $(static_output) $(objs)

$(shared_output): $(objs)
	$(ld) $(ldflags) -o $(shared_output) $(objs)

%.o: %.c
	$(cc) $(cflags) $< -c -o $@

clean:
	$(rm) -rf $(objs) $(all_outputs)
