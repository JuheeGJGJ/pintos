#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

struct semaphore mutex, wrt;
int readcount;
static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
	sema_init (&mutex, 1);
	sema_init (&wrt, 1);
	readcount = 0;
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* 20161578 added function - check user memory */
void check_multiple (void* esp, int n) {
	int i;

	for (i = 0 ; i < n ; i++) {
		if (!is_user_vaddr((void*)(esp + (4 * n)))) exit(-1);
	}
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  /* 20161578 */
	switch (*(unsigned*)(f->esp)) {
		case SYS_HALT : halt(); 
						break;
		
		case SYS_EXIT : check_multiple (f->esp, 1);
						exit ((int)*(int*)(f->esp + 4));
						break;

		case SYS_EXEC : check_multiple (f->esp, 1);
						f->eax = exec ((char*)*(int*)(f->esp + 4));
						break;

		case SYS_WAIT : check_multiple (f->esp, 1);
						f->eax = wait ((int)*(int*)(f->esp + 4));
						break;

		case SYS_READ : check_multiple (f->esp, 3);
						f->eax = read ((int)*(int*)(f->esp + 4), (void*)*(int*)(f->esp + 8), (unsigned)*(int*)(f->esp + 12));
						break;

		case SYS_WRITE : check_multiple (f->esp, 3);
						 f->eax = write ((int)*(int*)(f->esp + 4), (void*)*(int*)(f->esp + 8), (unsigned)*(int*)(f->esp + 12));
						 break;

		case SYS_FIBO : check_multiple (f->esp, 1);
						f->eax = fibonacci ((int)*(int*)(f->esp + 4));
						break;
		
		case SYS_SUM : check_multiple (f->esp, 4);
					   f->eax = sum_of_four_int ((int)*(int*)(f->esp + 4), (int)*(int*)(f->esp + 8), (int)*(int*)(f->esp + 12), (int)*(int*)(f->esp + 16));
					   break;

		case SYS_CREATE : check_multiple (f->esp, 2);
						  f->eax = create ((char*)*(int*)(f->esp + 4), (unsigned)*(int*)(f->esp + 8));
						  break;

		case SYS_REMOVE : check_multiple (f->esp, 1);
						f->eax = remove ((char*)*(int*)(f->esp + 4));
						break;
	
		case SYS_OPEN : check_multiple (f->esp, 1);
						f->eax = open ((char*)*(int*)(f->esp + 4));
						break;
	
		case SYS_FILESIZE : check_multiple (f->esp, 1);
					     	f->eax = filesize ((int)*(int*)(f->esp + 4));
					     	break;
	
		case SYS_SEEK : check_multiple (f->esp, 2);
					    seek ((int)*(int*)(f->esp + 4), (unsigned)*(int*)(f->esp + 8));
					    break;
		
		case SYS_TELL : check_multiple (f->esp, 1);
					     	f->eax = tell ((int)*(int*)(f->esp + 4));
					     	break;
	
		case SYS_CLOSE : check_multiple (f->esp, 1);
					     	close ((int)*(int*)(f->esp + 4));
					     	break;
	
		default : thread_exit(); break;
	}
//  printf ("system call!\n");
//  thread_exit ();
}

void halt (void) {
	shutdown_power_off();
}

void exit (int status) {
	printf ("%s: exit(%d)\n", thread_name(), status);
	thread_current() -> exit_status = status;

	int i;
	for (i = 0 ; i < 128 ; i++) {
		if (thread_current() -> file[i] != NULL) close(i);
	}

	thread_exit();
}

pid_t exec (const char *cmd_line) {
	return process_execute (cmd_line);
}

int wait (pid_t pid) {
	return process_wait (pid);
}

int read (int fd, void *buffer, unsigned size) {
	int i = -1;

/*	sema_down (&mutex);
	readcount++;
	if (readcount == 1) sema_down (&wrt);
	sema_up (&mutex);
*/
	if (fd == 0) {
		for (i = 0 ; i < size ; i++) {
			((char*)buffer)[i] = input_getc();
			if (((char*)buffer)[i] == '\0') break;
		}
	}
	else if (fd > 2) {
		if (!is_user_vaddr(buffer)) exit(-1);
		i = file_read (thread_current() -> file[fd], buffer, size);
	}

/*	sema_down (&mutex);
	readcount--;
	if (readcount == 0) sema_up (&wrt);
	sema_up (&mutex);
*/	
	return i;
}

int write (int fd, const void *buffer, unsigned size) {
	int r = -1;

//	sema_down (&wrt);
	
	if (fd == 1) {
		putbuf((char*)buffer, size);
		r = size;
	}
	else if (fd > 2) {
		r = file_write (thread_current() -> file[fd], buffer, size);
	}

//	sema_up (&wrt); 

	return r;
}

int fibonacci (int n) {
	int n1 = 1, n2 = 1, n3 = 1;
	int i;

	if (n < 0) return 0;
	
	for (i = 3 ; i <= n ; i++) {
		n3 = n1 + n2;
		n1 = n2;
		n2 = n3;
	}

	return n3;
}

int sum_of_four_int (int a, int b, int c, int d) {
	return (a + b + c + d);
}

bool create (const char *file, unsigned initial_size){
	if (file == NULL) exit(-1);
	return filesys_create (file, initial_size);
} 

bool remove (const char *file) {
	return filesys_remove (file);
}

int open (const char *file) {
	if (file == NULL) exit(-1);

	int i;
	int r = -1;
	struct file* open_file;
	
	open_file = filesys_open (file);

	if (open_file != NULL) {
		for (i = 3 ; i < 128 ; i++) {
			if (thread_current()->file[i] == NULL) {
				thread_current()->file[i] = open_file;

				if (strcmp (thread_current() -> name, file) == 0) 
					file_deny_write (thread_current()->file[i]);
				
				r = i;
				break;
			}
		}
	}

	return r;
}

int filesize (int fd) {
	return (int)file_length (thread_current() -> file[fd]);
}

void seek (int fd, unsigned position) {
	file_seek (thread_current() -> file[fd], position);
}

unsigned tell (int fd) {
	return file_tell (thread_current() -> file[fd]);
}

void close (int fd) {
	if (thread_current() -> file[fd] == NULL) exit(-1); //file doesn't exist

	file_close (thread_current() -> file[fd]);
	thread_current() -> file[fd] = NULL; //set file[fd]
}
