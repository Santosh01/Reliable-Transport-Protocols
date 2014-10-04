all : abt gbn sr

abt : abt.c
	gcc abt.c -o abt
gbn : gbn.c
	gcc gbn.c -o gbn
sr : sr.c
	gcc sr.c -o sr

clean :
	rm -f *.o abt gbn sr

	
	
	
	
	

