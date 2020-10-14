#include <console.h>
#include <debug.h>
#include <inttypes.h>
#include <limits.h>
#include <random.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "userprog/syscall.h"
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "devices/shutdown.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "threads/synch.h"
#include "threads/malloc.h"

static void syscall_handler (struct intr_frame *);

static int exit_status = 0x00;

struct lock _lock;
struct lock process_lock;

int get_exit_code(void);
void abort(void);
static int sys_exit (int);
static inline bool check_ptr(const void*);
static int sys_create (const char*, unsigned);
static int sys_remove (const char*);
static int sys_halt (void);
static struct file_system *list_search (int, struct list*);
static int sys_open (const char*);
static int sys_filesize (int);
static int sys_close (int);
static int sys_close (int);
static int sys_read (int, void*, unsigned);
static int sys_write (int, const void*, unsigned);
static int sys_exec (const char*);
static int sys_wait (int);

void
syscall_init (void) 
{
lock_init(&_lock);
lock_init(&process_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
int nr = *(int*) f->esp;
int* args = (int*) f->esp +1;
int arg[3] = {*((int*)f->esp+1),*((int*)f->esp+2), *((int*)f->esp+3) };

struct allCMD
{
int cmd;
int argc;
int (*func)(int, int, int);
};

static const struct allCMD allcmdTable[] = 
{
{SYS_EXIT, 1, sys_exit},
{SYS_HALT, 0, sys_halt},
{SYS_CREATE, 2, sys_create},
{SYS_REMOVE, 1, sys_remove},
{SYS_OPEN, 1, sys_open},
{SYS_CLOSE, 1, sys_close},
{SYS_FILESIZE, 1, sys_filesize},
{SYS_READ, 3, sys_read},
{SYS_WRITE, 3, sys_write},
{SYS_EXEC, 1, sys_exec},
{SYS_WAIT, 1, sys_wait},
};
if(!check_ptr(args) || !check_ptr(args+1) || !check_ptr(args+2)) abort();
for(int i=0; i<11; i++)
if(nr == allcmdTable[i].cmd) {lock_acquire(&_lock);f->eax = allcmdTable[i].func(arg[0], arg[1], arg[2]); lock_release(&_lock); break;}
}

static int sys_exit (int exit_code){
printf("%s: exit(%d)\n",thread_current()->name,exit_code);
thread_exit();
NON_REACHED();
return 0;
}

static int sys_exec (const char* cmd_line){
if(cmd_line == 0 || !check_ptr(cmd_line)) return -1;
tid_t tid = process_execute(cmd_line);
return tid;
}

static int sys_wait (int pid){
return process_wait(pid);
}

void abort (void){
sys_exit(-1);
NON_REACHED();
}

int get_exit_code(void){
return exit_status;
}

static inline bool check_ptr(const void* ptr){
return (ptr<PHYS_BASE && pagedir_get_page(thread_current()->pagedir, ptr)!=0);
}

static int sys_create (const char *file, unsigned initial_size){
if(file == 0 || !check_ptr(file)) abort();
int code= filesys_create(file, initial_size);
return code;
}

static int sys_remove (const char *file){
if(file == 0 || !check_ptr(file)) abort();
int code= filesys_remove(file);
return code;
}

static int sys_halt (void){
shutdown_power_off();
NON_REACHED();
return 0;
}

static struct file_system *list_search (int fd, struct list* l)
{
for (struct list_elem *e=list_begin(l);
e != list_end (l); e = list_next(e))
if (list_entry(e, struct file_system,elem)->fd == fd)
return list_entry(e, struct file_system,elem);
return 0;
}

static int sys_open (const char *file){
if(file == 0 || !check_ptr(file)) abort();
struct file* fin = filesys_open(file);
if(!fin)
return -1;
struct thread* cur = thread_current();
struct file_system* NFS = (struct file_system*)malloc(sizeof(struct file_system));
int sfd = NFS->fd = cur->all_n++; NFS->fp = fin;
list_push_front(&cur->FS, &NFS->elem);// stack
return sfd;
}

static int sys_close (int fd){
if(fd>=thread_current()->all_n || fd<2) return -1;
struct file_system* temp = list_search(fd, &thread_current()->FS);
if(temp==0) return -1;
file_close(temp->fp);
list_remove(&temp->elem);
free(temp);
return 0;
}

static int sys_filesize (int fd){
struct file_system* temp = list_search(fd, &thread_current()->FS);
if(temp == 0) return -1;
return file_length(temp->fp);
}

static int sys_read (int fd, void* buffer, unsigned size){
if(!check_ptr(buffer)) abort();
if(fd == 0){
for(int i = 0; i<size; i++)
*((char*)buffer+i) = (char)input_getc();
return size;
}

struct file_system* temp = list_search(fd, &thread_current()->FS);
if(temp==0) return -1;
return file_read(temp->fp, buffer, size);
}

static int sys_write (int fd, const void* buffer, unsigned size){
if(!check_ptr(buffer)) abort();
if(fd == 1){ putbuf( (const char**)buffer, (size_t*) size); return 0;}
struct file_system* temp = list_search(fd, &thread_current()->FS);
if(temp==0) return -1;
return  file_write(temp->fp, buffer, size);
}



