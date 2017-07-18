// RUN: %check_clang_tidy %s mpi-buffer-deref %t -- -- -I %S/Inputs/mpi-type-mismatch

#include "mpimock.h"

void negativeTests() {
  char *buf;
  MPI_Send(&buf, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: buffer is insufficiently dereferenced: pointer->pointer [mpi-buffer-deref]

  unsigned **buf2;
  MPI_Send(buf2, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD);
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: buffer is insufficiently dereferenced: pointer->pointer

  short buf3[1][1];
  MPI_Send(buf3, 1, MPI_SHORT, 0, 0, MPI_COMM_WORLD);
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: buffer is insufficiently dereferenced: array->array

  long double _Complex *buf4[1];
  MPI_Send(buf4, 1, MPI_C_LONG_DOUBLE_COMPLEX, 0, 0, MPI_COMM_WORLD);
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: buffer is insufficiently dereferenced: pointer->array

  std::complex<float> *buf5[1][1];
  MPI_Send(&buf5, 1, MPI_CXX_FLOAT_COMPLEX, 0, 0, MPI_COMM_WORLD);
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: buffer is insufficiently dereferenced: pointer->array->array->pointer

  int *buf6;
  MPI_File fh;
  MPI_File_write(fh, &buf6, 1, MPI_INT, MPI_STATUS_IGNORE);
  // CHECK-MESSAGES: :[[@LINE-1]]:22: warning: buffer is insufficiently dereferenced: pointer->pointer

  int *buf7;
  MPI_File_read(fh, &buf7, 1, MPI_INT, MPI_STATUS_IGNORE);
  // CHECK-MESSAGES: :[[@LINE-1]]:21: warning: buffer is insufficiently dereferenced: pointer->pointer

  float **buf8;
  MPI_File_write_at(fh, 0, buf8, 1, MPI_FLOAT, MPI_STATUS_IGNORE);
  // CHECK-MESSAGES: :[[@LINE-1]]:28: warning: buffer is insufficiently dereferenced: pointer->pointer

  float **buf9;
  MPI_File_read_at(fh, 0, buf9, 1, MPI_FLOAT, MPI_STATUS_IGNORE);
  // CHECK-MESSAGES: :[[@LINE-1]]:27: warning: buffer is insufficiently dereferenced: pointer->pointer

  int *buf10;
  MPI_File_iwrite(fh, &buf9, 1, MPI_INT, MPI_STATUS_IGNORE);
  // CHECK-MESSAGES: :[[@LINE-1]]:23: warning: buffer is insufficiently dereferenced: pointer->pointer

  int *buf11;
  MPI_File_iread(fh, &buf11, 1, MPI_INT, MPI_STATUS_IGNORE);
  // CHECK-MESSAGES: :[[@LINE-1]]:22: warning: buffer is insufficiently dereferenced: pointer->pointer
  
  int *buf12;
  MPI_Request req;
  MPI_File_iread_at(fh, 0, &buf12, 1, MPI_INT, &req);
  // CHECK-MESSAGES: :[[@LINE-1]]:28: warning: buffer is insufficiently dereferenced: pointer->pointer
  
  int **buf13;
  MPI_File_iwrite_at(fh, 0, buf13, 1, MPI_INT, &req);
  // CHECK-MESSAGES: :[[@LINE-1]]:29: warning: buffer is insufficiently dereferenced: pointer->pointer
   
  int *buf14;
  MPI_File_read_shared(fh, &buf14, 1, MPI_INT, MPI_STATUS_IGNORE);
  // CHECK-MESSAGES: :[[@LINE-1]]:28: warning: buffer is insufficiently dereferenced: pointer->pointer
  
  int **buf15;
  MPI_File_write_shared(fh, buf15, 1, MPI_INT, MPI_STATUS_IGNORE);
  // CHECK-MESSAGES: :[[@LINE-1]]:29: warning: buffer is insufficiently dereferenced: pointer->pointer
  
  int buf16[1][3];
  MPI_File_iread_at(fh, 0, buf16, 1, MPI_INT, &req);
  // CHECK-MESSAGES: :[[@LINE-1]]:28: warning: buffer is insufficiently dereferenced: array->array 
  
  int **buf17;
  MPI_File_iwrite_at(fh, 0, buf17, 1, MPI_INT, &req);
  // CHECK-MESSAGES: :[[@LINE-1]]:29: warning: buffer is insufficiently dereferenced: pointer->pointer
}

void positiveTests() {
  char buf;
  MPI_Send(&buf, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

  unsigned *buf2;
  MPI_Send(buf2, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD);

  short buf3[1][1];
  MPI_Send(buf3[0], 1, MPI_SHORT, 0, 0, MPI_COMM_WORLD);

  long double _Complex *buf4[1];
  MPI_Send(*buf4, 1, MPI_C_LONG_DOUBLE_COMPLEX, 0, 0, MPI_COMM_WORLD);

  long double _Complex buf5[1];
  MPI_Send(buf5, 1, MPI_C_LONG_DOUBLE_COMPLEX, 0, 0, MPI_COMM_WORLD);

  std::complex<float> *buf6[1][1];
  MPI_Send(*buf6[0], 1, MPI_CXX_FLOAT_COMPLEX, 0, 0, MPI_COMM_WORLD);

  // Referencing an array with '&' is valid, as this also points to the
  // beginning of the array.
  long double _Complex buf7[1];
  MPI_Send(&buf7, 1, MPI_C_LONG_DOUBLE_COMPLEX, 0, 0, MPI_COMM_WORLD);

  int buf8;
  MPI_File fh;
  MPI_File_write(fh, &buf8, 1, MPI_INT, MPI_STATUS_IGNORE);

  int buf9;
  MPI_File_read(fh, &buf9, 1, MPI_INT, MPI_STATUS_IGNORE);

  float buf10;
  MPI_File_write_at(fh, 0, &buf8, 1, MPI_FLOAT, MPI_STATUS_IGNORE);

  float buf11;
  MPI_File_read_at(fh, 0, &buf8, 1, MPI_FLOAT, MPI_STATUS_IGNORE);

  int buf12;
  MPI_File_iwrite(fh, &buf12, 1, MPI_INT, MPI_STATUS_IGNORE);

  int buf13;
  MPI_File_iread(fh, &buf13, 1, MPI_INT, MPI_STATUS_IGNORE);

  int *buf14;
  MPI_Request req;
  MPI_File_iread_at(fh, 0, buf14, 1, MPI_INT, &req);
  
  int *buf15;
  MPI_File_iwrite_at(fh, 0, buf15, 1, MPI_INT, &req);
   
  int buf16;
  MPI_File_read_shared(fh, &buf16, 1, MPI_INT, MPI_STATUS_IGNORE);
  
  int buf17;
  MPI_File_write_shared(fh, &buf17, 1, MPI_INT, MPI_STATUS_IGNORE);
  
  int buf18[1][3];
  MPI_File_iread_at(fh, 0, buf18[0], 1, MPI_INT, &req);
  
  int *buf19;
  MPI_File_iwrite_at(fh, 0, buf19, 1, MPI_INT, &req);
}
