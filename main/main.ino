// Comment this line when compiling for a slave device
#define COMPILE_FOR_MASTER_DEVICE

#ifdef COMPILE_FOR_MASTER_DEVICE
#include "Master.hpp"
#else
#include "Slave.hpp"
#endif
