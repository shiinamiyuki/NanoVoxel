#ifndef NANOVOXEL_COMPAT_H
#define NANOVOXEL_COMPAT_H

#ifdef OPENCL_KERNEL
#include "compat_opencl.h"
#else
// just for IDE to check the code
#include "compat_cpu.h"
#pragma pack(push,1)
#endif
typedef Float4 Spectrum;
typedef int Seed;

#endif