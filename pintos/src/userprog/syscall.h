#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <list.h>

struct file_system {
int fd;
struct file* fp;
struct list_elem elem;
};

struct child* list_search_c (int, struct list*);
void syscall_init (void);
void abort(void);
struct lock process_lock;
#endif /* userprog/syscall.h */
