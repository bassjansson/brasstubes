// Change this number to compile for a specific device
// 0 = Master device
// 1 to 5 = Slave devices
#define DEVICE_NUMBER 1

// This selects the code depending on the device type
#if (DEVICE_NUMBER == 0)
#include "Master.hpp"
#else
#include "Slave.hpp"
#endif
