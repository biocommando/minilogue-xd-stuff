CC:=gcc
LIBS:=-lm
CFLAGS:=-Wall

test: test.out
	./test.out

test.out:
	$(CC) -o $@ test.c wav_handler.c $(CFLAGS) $(LIBS)

clean:
	rm -rf test.out
	rm -rf *.wav
