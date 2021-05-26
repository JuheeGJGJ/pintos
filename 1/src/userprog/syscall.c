#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
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
	thread_exit();
}

pid_t exec (const char *cmd_line) {
	return process_execute (cmd_line);
}

int wait (pid_t pid) {
	return process_wait (pid);
}

int read (int fd, void *buffer, unsigned size) {
	int i = 0;
	if (fd == 0) {
		for (i = 0 ; i < size ; i++) {
			((char*)buffer)[i] = input_getc();
			if (((char*)buffer)[i] == '\0') break;
		}
	}

	return i;
}

int write (int fd, const void *buffer, unsigned size) {
	if (fd == 1) {
		putbuf((char*)buffer, size);
		return size;
	}

	return -1;
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
