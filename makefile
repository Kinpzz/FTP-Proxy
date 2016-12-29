proxy: token.o ftp_proxy.c
	gcc -o proxy ftp_proxy.c token.o
token: token.c
	gcc -c token.c
clean:
	rm *.o
