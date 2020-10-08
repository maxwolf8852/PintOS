#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

static int exit_status = 0x00;

int get_exit_code(void){
return exit_status;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  if ( *(int*) f->esp == SYS_WRITE) {
 putbuf( ((const char**) f->esp)[2], ((size_t*) f->esp)[3]);
 return;
 }
  if (*(int*) f->esp == SYS_EXIT) {
 exit_status = ((size_t*) f->esp)[1];
 thread_exit();
 }
else {
printf ("system call!\n");
 thread_exit ();
}

}
