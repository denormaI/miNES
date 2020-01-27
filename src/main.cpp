#include "mines_sys.h"
#include <iostream>

int main(int argc, char* argv[])
{
	if (argc <= 1)
	{
		std::cout << "usage: " << argv[0] << " <rom path> <optional arguments>\n";
		std::cout << "optional:\n\t--stretch\tdon't preserve video aspect ratio\n";
		return 0;
	}

	try
	{
		bool keep_ppu_aspect = true;

		std::string rom_path = argv[1];

		for (int i = 2; i < argc; i++)
		{
			if (std::string(argv[i]) == "--stretch")
				keep_ppu_aspect = false;
		}

		mines_sys sys(rom_path, keep_ppu_aspect);
		sys.entry();
	}
	catch (mines_exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
