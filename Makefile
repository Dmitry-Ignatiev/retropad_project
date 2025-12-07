CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -municode -DUNICODE -D_UNICODE
LDFLAGS = -municode -mwindows -lcomdlg32 -lcomctl32 -lgdi32 -lshell32 -luser32

# DEFINED ONCE: All your source files, including the new printer.c
SRCS = retropad.c ui.c file_io.c dialogs.c printer.c
OBJS = $(SRCS:.c=.o) resource.o

.PHONY: all clean run

all: retropad.exe

retropad.exe: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

# Generic rule for compiling .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile the resource file
resource.o: retropad.rc resource.h
	windres retropad.rc -o resource.o

clean:
	rm -f $(OBJS) retropad.exe

run: retropad.exe
	./retropad.exe