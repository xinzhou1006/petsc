/* $Id: ptime.h,v 1.10 1995/12/05 05:03:05 bsmith Exp bsmith $ */

#if !defined(__PTIME_PACKAGE)
#define __PTIME_PACKAGE

#if defined(PARCH_IRIX) && defined(__cplusplus)
struct timeval {
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* and microseconds */
};

struct timezone {
        int     tz_minuteswest; /* minutes west of Greenwich */
        int     tz_dsttime;     /* type of dst correction */
};
extern "C" {
extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
}
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#if defined(PARCH_sun4)
extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
#endif
#if defined(PARCH_solaris) && defined(__cplusplus)
extern "C" {
extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
}
#endif

/*
    Macros for timing. In the future some of these may be 
    machine dependent versions
*/

/*M
   PetscTime - Returns the current time of day in seconds.  

   Output Parameter:
.  v - time counter

   Synopsis:
   PetscTime(double v)

   Usage: 
     double v;
     PetscTime(v);
     .... perform some calculation ...
     PetscTimeElapsed(v); 
     printf("Time for operation %g\n",v);

   Notes:
   Since the PETSc libraries incorporate timing of phases and operations, 
   PetscTime() is intended only for timing of application codes.  
   The options database commands -log, -log_summary, and -log_all activate
   PETSc library timing.  See the users manual for further details.

.seealso:  PetscTimeSubtract(), PetscTimeAdd(), PetscTimeElapsed()

.keywords:  Petsc, time
M*/
#define PetscTime(v)         {struct timeval _tp; \
                             gettimeofday(&_tp,(struct timezone *)0);\
                             (v)=((double)_tp.tv_sec)+(1.0e-6)*(_tp.tv_usec);}

/*M
   PetscTimeSubtract - Subtracts the current time of day (in seconds) from
   the value v.  

   Input Parameter:
.  v - time counter

   Output Parameter:
.  v - time counter (v = v - current time)

   Synopsis:
   PetscTimeSubtract(double v)

   Notes:
   Since the PETSc libraries incorporate timing of phases and operations, 
   PetscTimeSubtract() is intended only for timing of application codes.  
   The options database commands -log, -log_summary, and -log_all activate
   PETSc library timing.  See the users manual for further details.

.seealso:  PetscTime(), PetscTimeAdd() PetscTimeElapsed()

.keywords:  Petsc, time, subtract
M*/

#define PetscTimeSubtract(v) {struct timeval _tp; \
                             gettimeofday(&_tp,(struct timezone *)0);\
                             (v)-=((double)_tp.tv_sec)+(1.0e-6)*(_tp.tv_usec);}
/*M
   PetscTimeAdd - Adds the current time of day (in seconds) to the value v.  

   Input Parameter:
.  v - time counter

   Output Parameter:
.  v - time counter (v = v + current time)

   Synopsis:
   PetscTimeAdd(double v)

   Notes:
   Since the PETSc libraries incorporate timing of phases and operations, 
   PetscTimeAdd() is intended only for timing of application codes.  
   The options database commands -log, -log_summary, and -log_all activate
   PETSc library timing.  See the users manual for further details.

.seealso:  PetscTime(), PetscTimeSubtract(), PetscTimeElapsed() 

.keywords:  Petsc, time, add
M*/
#define PetscTimeAdd(v)      {struct timeval _tp; \
                             gettimeofday(&_tp,(struct timezone *)0);\
                             (v)+=((double)_tp.tv_sec)+(1.0e-6)*(_tp.tv_usec);}

/*M
   PetscTimeElapsed - Returns the time from the call to PetscTime().  

   Output Parameter:
.  v - time counter

   Synopsis:
   PetscTimeElapsed(double v)

   Usage: 
     double v;
     PetscTime(v);
     ... perform some calculation ..
     PetscTimeElapsed(v); 
     printf("Time for operation %g\n",v);

   Notes:
   Since the PETSc libraries incorporate timing of phases and operations, 
   PetscTimeElapsed() is intended only for timing of application codes.  
   The options database commands -log, -log_summary, and -log_all activate
   PETSc library timing.  See the users manual for further details.

.seealso:  PetscTimeSubtract(), PetscTimeAdd(), PetscTim()

.keywords:  Petsc, time
M*/
#define PetscTimeElapsed(v)  {PetscTimeSubtract(v); v = -(v); }

#endif


