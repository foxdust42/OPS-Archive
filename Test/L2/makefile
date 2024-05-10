all:
    main

main: 
    gcc -Wall -fsanitize=address,undefined -o main.o main.c

.PHONY:
    all clean

clean:
    rm *.o