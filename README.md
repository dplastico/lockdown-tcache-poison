# lockdown-tcache-poison

La idea de este repo es entregar material (y contexto) para el desarrllo del taller "Intro a heap". EL taller forma parte de lockdown0x0 (link aca) y es de uso exclusivo para los participantes. Ademas su uso es estrictamente educacional y no soy responsable del uso que se le de.

# Disclaimer

Es necesario también considerar que el siguiente texto está parcialmente redactado por mi, lo cual implica posibles “typos” y también un fuerte uso de “spanglish” esto Por que a mi parecer hay conceptos que no vale la pena traducir y en el sentido de este taller, es más útil referirse a los mismos de esta forma.

# Requisitos

**LINUX**

Cualquier versión, estaré utilizando ubuntu 20.04 pero cualquiera debería servir.

**GDB**

> sudo apt install gdb

**PWNTOOLS**

> pip install pwntools

**Un editor de texto**

vim, nano, vscode, sublime, mousepad, etc.

# Instrucciones

Los programas (binarios) a utilizar en el taller estan ubicados en la carpeta raiz de este repo, y la estructura de directorios es utilizada para llamar el "linker" y version de libc (ld-2.28.so y libc.so.6 ('../') por lo cual deben mantenerse en dicha carpeta
Si deseas reparar el rpath del binario para que ocupe otro path puedes utilizar la herramienta patchelf, pero para efectos de este ejercicio no es mandatorio

> patchelf --set-interpreter /path/to/ld/ld-.so --set-rpath /path/to/libc/

# ¿Por qué GLIBC?

GLIBC es la libreria mas grande usada en Linux, el OS más usado ampliamente (servidores) Por lo mismo la implementación de malloc en libc es una de las más protegidas (si no la mas) y por lo mismo aprender a explotar el HEAP en glibc presenta una gran ventaja en el mundo de hoy. 

Todos los ejemplos de este taller están hechos usando glibc 2.28 (por defecto en ubuntu 18.04) la razón de elegir esta version es por que sigue con soporte, es ampliamente visible hoy, y nos da un poco más de espacio para explicar las vulnerabilidades que la actual 2.31 (en ubuntu 20) De todas formas las mismas técnicas son aplicables para 2.31 con algunas modificaciones.

# Que es el HEAP?

HEAP es un espacio de memoria que un programa puede asignar para su uso. A diferencia del “stack” la memoria en el HEAP puede ser dinámicamente asignada. Esto significa que el programa puede “pedir” y “liberar” memoria de la región del heap, cada vez que lo solicite (en el contexto del programa, o sea, en el código). Por otro lado el heap es un segmento “global” Esto quiere decir que no está ubicado en la función que lo ejecuta (como el stack) Y puede ser accedido y modificado en diferentes instancias del “runtime”. Esto se logra por medio de punteros, los cuales referencian la data asignada y/o liberada, de esta forma se puede mantener una “mapa” del heap, permitiendo acceder a los diferentes recursos del mismo.

La asignación y liberación de memoria se puede explicar de manera sencilla utilizando dos funciones malloc() y free()

# Malloc

Malloc es el nombre que se le da al “asignador” de memoria en GLIBC y es una colección de funciones y metadata que se usan para proveer a un proceso en “runtime” acceso a memoria dinámica. Esta metadata se encuentra presente en “chunks” o bloques y en “Arenas” Una Arena contiene una estructura (struct) con información de un determinado heap. 
Malloc cuando es invocado, usa como argumento un valor, el cual determina el tamaño, del bloque a asignar. y luego de ejecutarse retorna un puntero a este bloque. Podemos ver un ejemplo del uso de malloc en el siguiente código

```c

    char *buf_A; //declarando un buffer A

    buf_A = malloc(24); //se asignan 24 bytes a buf_A
```
# Free 

Free por otro lado, es una función que “libera” el uso de un bloque específico de memoria asignado por malloc. Free toma como argumento un puntero al bloque mencionado (esto puede ser el retorno de malloc)
Podemos ver un ejemplo de esto en el siguiente codigo
```c
    char *buf_A; //declarando un buffer A

    //asignacion de memoria   
    buf_A = malloc(24); //se asignan 24 bytes a buf_A

    //liberacion de memoria usando free
    free(buf_a);
```

# ¿Qué es un “chunk” (Bloque)?

En spanglish un “chunk” es un bloque de memoria que se asigna vía malloc, este bloque contiene la información (metadata) de el mismo. Una de las propiedades del HEAP es que sus bloques contienen la información de sí mismos. Esto permite crear listas simples o enlazadas de información en la memoria y de esta forma cada “chunk” puede contener la información de su tamaño, bloque previo, siguiente, si está liberado o no, etc.
Diagrama de un chunk:

    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk, if unallocated (P clear)  |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of chunk, in bytes                     |A|M|P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             User data starts here...                          .
            .                                                               .
            .             (malloc_usable_size() bytes)                      .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             (size of chunk, but used for application data)    |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of next chunk, in bytes                |A|0|1|
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            
            **source** https://heap-exploitation.dhavalkapil.com/diving_into_glibc_heap/malloc_chunk

# ¿Qué es el tcache?

En GLIBC 2.26 tcache fue introducido, La idea de tcache, es ganar rendimiento. Esto debido a que el tcache (thread cache) es parte de cada “thread” y único para el mismo. EL tcache es una estructura (struct) que se comporta como una “Arena” guardando información de cada “chunk” en el thread que es liberado para tener un rápido acceso en caso de requerirse. Todos los “chunk” de tamaño 0x20 hasta 0x400 son parte del tcache, y usan este cache rápido para liberar y asignar “chunks”.

Esta mejora en rendimiento trajo consigo algunos problemas de seguridad que habían sido previamente solucionados con parches anteriores, reviviendo ciertas técnicas de explotación.

![tcache](https://i.imgur.com/O41t29O.png)
**source** [Heap Bible by Max Kamper](https://www.udemy.com/course/linux-heap-exploitation-part-1/)

# Tcache poison

La idea detrás de “tcache poison” es “envenenar” (poison = veneno en inglés) el tcache. Esto lo logramos haciendo que la asignación del heap retorne a un puntero cualquiera seleccionado por el atacante desde malloc. Esto puede ser ejecutado vía UAF (use after free) o,  DF (double free)

# Ejercicios

Los ejercicios de este repositorio están hechos para seguir de manera guiada o proactiva, contienen bugs directos y evitan al máximo la posibilidad de ingeniería inversa para así poder concentrar el ataque directamente en la explotación.

Cualquier duda consultarme a @dplastico

# Referencias y recursos
[How2heap](https://github.com/shellphish/how2heap)

[Mathias F Rorvik master](https://www.duo.uio.no/bitstream/handle/10852/69062/7/mymaster.pdf)

[tcache poison paper de Silvio Cesare](https://drive.google.com/file/d/1XpdruvtC1qW0OKLxO8FaqU9XCl8O_SON/view)

[Azeria HEAP xpl, para ARM, pero los mismos conceptos](https://azeria-labs.com/heap-exploitation-part-1-understanding-the-glibc-heap-implementation/)

[c4e](https://c4ebt.github.io/)

[Max Kamper ofrece cursos de explotacion de HEAP y por supuesto ROPemporium](https://ropemporium.com)

[sitio personal](https://github.dplastico.io)
