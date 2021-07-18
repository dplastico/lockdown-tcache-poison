#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>

int main(){

    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);

    char *buf_A; //declarando un buffer A
    char *buf_B; //declarando un buffer B
    char *buf_C; //declarando un buffer C

    //ejemplo de uso de malloc
    printf("Ejemplo de asignacion de memoria");
    //asignacion de memoria   
    buf_A = malloc(24); //se asignan 24 bytes a buf_A
    buf_B = malloc(24); //se asignan 24 bytes a buf_B

    printf("pointer a Buf_A %p \n", buf_A);
    printf("pointer a Buf_B %p \n", buf_B);

    // copiamos informacion a buf_A
    strcpy(buf_A, "AAAAAAAA");
    //liberamos buf_A
    free(buf_A);
    //asignamos un chunk fuera del tcache
    buf_C = malloc(0x420);
    //liberamos B
    free(buf_B);
    //reasignamos buf_A
    buf_A = malloc(24);


    //liberamos al topchunk
    free(buf_C);

    return 0;


}