#pragma once

#include "types.h"

#include <string>
#include <map>
#include <memory>

class nes_device;

// device name to device object map.
typedef std::map<std::string, std::shared_ptr<nes_device>> nes_device_map_t;

class nes_device
{
public:
	/*
		A class derived from nes_device may have dependencies on other nes_device objects. To
		implement this, any nes_device may have nes_device's "attached" to it, that is,
		passed wrapped in a shared_ptr and converted to the appropriate type by the
		implementation based on the name. All devices may have shared ownership so shared_ptr
		is necessary. The implementation can retrieve the devices it requires from nes_device_map_t.

		Relying on devices knowing the names of the devices they're dependent on is quite
		error-prone, but if an unknown name is used this can be easily checked for. For example,
		if a device cannot retrieve a needed device object it can just throw an exception.
		TODO: Checking for this at compile time would be better.
	*/
	virtual void attach_devices(nes_device_map_t device_map) = 0;

protected:
	nes_device() {}
	virtual ~nes_device() {}

private:
    nes_device(nes_device const&) = delete;
    nes_device& operator=(nes_device const&) = delete;		
};
