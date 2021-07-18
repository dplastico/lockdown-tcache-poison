#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

uint idx = 0;

void win(){
    char flag[100];
    FILE *f = fopen("flag.txt", "r");
    if(f == NULL){
        puts("Flag no esta!");
        exit(0);
    }
    fgets(flag,100,f);
    printf("flag : %s", flag);
}

void buffering(){
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);
}

void menu(){
    puts("1. Allocate");
    puts("2. Delete");
    puts("3. Exit");
    puts("> ");
}

int add(char **m_array){
    ulong size;

    puts("size : ");
    scanf("%lu", &size);
    if(size >= 0x88){
        printf("El size debe ser meno a 0x88\n");
        return 0;
        }
    m_array[idx] = malloc(size);
    puts("data : ");
    read(0,m_array[idx],size);
    idx ++;
}

int delete(char **m_array){
    ulong n;

    puts("index : ");
    scanf("%ld", &n)    ;
    if (n > idx){
        puts("no se ha asignado ");
        return 0;
        }
    free(m_array[n]);
    idx ++;
}

void main(){

    char buf[4];
    char *m_array [6];

    m_array[0] = (char *)0x00;      
    m_array[1] = (char *)0x00;      
    m_array[2] = (char *)0x00;      
    m_array[3] = (char *)0x00;      
    m_array[4] = (char *)0x00;
    m_array[5] = (char *)0x00;

    buffering();
    while(1){
        menu();
        read(0, buf, sizeof(buf));
        switch(atoi(buf)){
            case 1:
                    add(m_array);
                    break;
            case 2:
                    delete(m_array);
                    break;
            case 3:
                    puts("Saliendo");
                    exit(0);
                    break;
            default:
                    puts("Invalid option");
                    break;

        }

    }
}
