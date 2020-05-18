A benchmark suite for System V shared memory
============================================

Benchmarks:
1) HOTSPOT:
This test provides evaluation of the threshing prone application. 
For this type of applications, there can be some hot areas of the shared memory, which are simultaneously required by different processes.

2) ME (Multral Exclusion):
This test utilizes shared memory based mutral exclusion to provide exclusive access the share memory.

3) RAP (Random Access Pattern):
This test provids evaluation of the random access pattern when using the shared memory.

4) BL (Block Lock):
This test evaluates the access of the continuous memory blocks. In this test, the updates of each block can be fully shared among processes.

5) SPLASH2:
This is a modified version of the Stanford Parallel Applications for Shared-Memory (SPLASH), supporting System V IPC. Currently, only FFT, LU, and RADIX kernels are supported.

## **Version**
* 0.0.1

## **License**
* shmbench is released under the terms of the MIT License.
