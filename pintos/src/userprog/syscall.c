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

/*Faster 'cause uses processor's memory management unit */

#define INTBYTE 4

static void syscall_handler (struct intr_frame *);

static int exit_status = 0x00;

struct lock _lock;
struct lock process_lock;
int get_exit_code(void);
void abort(void);
static int sys_exit (int);
static int sys_create (const char*, unsigned);
static int sys_remove (const char*);
static int sys_halt (void);
static struct file_system *list_search (int, struct list*);
static int sys_open (const char*);
static int sys_filesize (int);
static int sys_close (int);
static int sys_read (int, void*, unsigned);
static int sys_write (int, const void*, unsigned);
static int sys_exec (const char*);
static int sys_wait (int);
#ifdef SPEEDMODE
static int get_user (const uint8_t*); //from pintos documentation
static void get_from_stack (void*, const uint8_t*, int);
#else
static inline bool check_ptr(const void*);
#endif
struct child* list_search_c (int, struct list*);

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
if(!is_user_vaddr((const void*)f->esp)) abort();
int nr;
get_from_stack(&nr, f->esp, INTBYTE*1);
int* arg = (int*) f->esp +1;
#ifdef SPEEDMODE
int args[3] = {0, 0, 0};
#else
int args[3] = {*((int*) f->esp +1), *((int*) f->esp +2), *((int*) f->esp +3)};
#endif

if (nr == SYS_EXIT) {
#ifdef SPEEDMODE
get_from_stack(args, arg, INTBYTE*1);
#else 
if(!check_ptr(arg)) abort();
#endif
sys_exit(*args);
}
else if (nr == SYS_WRITE){
#ifdef SPEEDMODE
get_from_stack(args, arg, INTBYTE*3);
#else
if(!check_ptr(arg) || !check_ptr(arg+1) || !check_ptr(arg+2)) abort();
#endif
if((int)(*args) == 1){ putbuf( ((const char**) f->esp)[2], ((size_t*) f->esp)[3]);}
else{
lock_acquire(&_lock);
f->eax = sys_write((int)(*args), (const void*)(*(args+1)), (unsigned)(*(args+2)));
lock_release(&_lock);}
}
else{

struct allCMD
{
int cmd;
int argc;
int (*func)(int, int, int);
};

static const struct allCMD allcmdTable[] = 
{
{SYS_HALT, 0, sys_halt},
{SYS_CREATE, 2, sys_create},
{SYS_REMOVE, 1, sys_remove},
{SYS_OPEN, 1, sys_open},
{SYS_CLOSE, 1, sys_close},
{SYS_FILESIZE, 1, sys_filesize},
{SYS_READ, 3, sys_read},
{SYS_EXEC, 1, sys_exec},
{SYS_WAIT, 1, sys_wait},
};

for(int i=0; i<9; i++)
if(nr == allcmdTable[i].cmd) {
#ifdef SPEEDMODE
get_from_stack(args, arg, INTBYTE*allcmdTable[i].argc);
#else
if(!check_ptr(arg) || !check_ptr((arg+1)) || !check_ptr((arg+2)) )abort();
#endif
lock_acquire(&_lock);
f->eax = allcmdTable[i].func(args[0], args[1], args[2]); 
lock_release(&_lock); 
break;}
}

}

#ifdef SPEEDMODE
static int
get_user (const uint8_t *uaddr)
{
if(uaddr>=PHYS_BASE) return -1;
int result;
asm ("movl $1f, %0; movzbl %1, %0; 1:"
: "=&a" (result) : "m" (*uaddr));
if(result == -1) abort();
return result;
}

static void get_from_stack(void* args, const uint8_t* stack, int bytes){
for(int i=0; i<bytes; i++){
*((uint8_t*)args+i) = get_user(stack+i);
if(*((uint8_t*)args+i) == -1) abort();
}
}
#else
static inline bool check_ptr(const void* ptr){
return (ptr<PHYS_BASE && pagedir_get_page(thread_current()->pagedir, ptr)!=0);
}
#endif

static int sys_exit (int exit_code){
struct thread* cur =  thread_current();
cur->exit_code = exit_code;
if(!list_empty(&cur->parent->child_list)){
struct child* c_cur = list_search_c(cur->tid, &cur->parent->child_list);
if(c_cur){
c_cur->is_alive = false;
c_cur->exit_code = exit_code;
if(cur->parent->wait_tid == cur->tid)
sema_up(&cur->parent->sema);
}
}

thread_exit();
NON_REACHED();
return 0;
}

static int sys_exec (const char* cmd_line){
#ifdef SPEEDMODE
if(!cmd_line || get_user((const uint8_t*)cmd_line) == -1) 
#else
if(!cmd_line || !check_ptr(cmd_line)) 
#endif
return -1;
tid_t tid = process_execute(cmd_line);
sema_down(&thread_current()->load_sema);
if(thread_current()->load_err){
struct child* c_cur = list_search_c(tid, &thread_current()->child_list);
if(c_cur){
list_remove(&c_cur->elem);
free(c_cur);
c_cur = 0;
}
return -1;
}
return tid;
}

static int sys_wait (int pid){
return process_wait(pid);
}

void abort (void){
sys_exit(-1);
NON_REACHED();
}


static int sys_create (const char *file, unsigned initial_size){
#ifdef SPEEDMODE
if(!file || get_user((const uint8_t*)file) == -1) 
#else
if(!file || !check_ptr(file))
#endif
abort();
int code= filesys_create(file, initial_size);
return code;
}

static int sys_remove (const char *file){
#ifdef SPEEDMODE
if(!file || get_user((const uint8_t*)file) == -1) 
#else
if(!file || !check_ptr(file))
#endif
abort();

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
#ifdef SPEEDMODE
if(!file || get_user((const uint8_t*)file) == -1) 
#else
if(!file || !check_ptr(file))
#endif
abort();
if(file == 0) abort();
struct file* fin = filesys_open(file);
if(!fin)
return -1;
struct thread* cur = thread_current();
struct file_system* NFS = (struct file_system*)malloc(sizeof(struct file_system));
int sfd = NFS->fd = cur->all_n++; NFS->fp = fin;
list_push_back(&cur->FS, &NFS->elem);// stack
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
#ifdef SPEEDMODE
//for(int i=0; i<size; i++)
if(get_user((uint8_t*)buffer)==-1) abort();

#endif
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
#ifdef SPEEDMODE
if(!buffer || get_user((const uint8_t*)buffer) == -1) 
#else
if(!buffer || !check_ptr(buffer))
#endif
abort();
//if(fd == 1){ putbuf( (const char**)buffer, (size_t*) size); return 0;}
struct file_system* temp = list_search(fd, &thread_current()->FS);
if(temp==0) return -1;
return  file_write(temp->fp, buffer, size);
}


struct child* list_search_c (int tid, struct list* l){
for(struct list_elem* e =list_begin(l);
e != list_end (l); e = list_next(e))
if (list_entry(e, struct child,elem)->tid == tid)
return list_entry(e, struct child,elem);
return 0;
}




