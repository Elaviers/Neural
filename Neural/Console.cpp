#include "Console.hpp"
#include "Sandbox.hpp"
#include "Digits.hpp"
#include <ELCore/Buffer.hpp>
#include <ELCore/String.hpp>
#include <ELSys/Debug.hpp>
#include <ELSys/System.hpp>
#include <iostream>

int Console::Run()
{
	System::OpenConsole();

	bool active = true;
	while (active)
	{
		String cmd;
		std::cout << "NEURAL>";
		std::getline(std::cin, cmd);

		Buffer<String> tokens = cmd.Split(" \r\n");
		if (tokens.GetSize() > 0)
		{
			String first = tokens[0].ToLower();

			if (first == "help")
			{
				std::cout <<
					"NEURAL v0.1\n"
					"-----------\n"
					"digits\t\tdigit recognition\n"
					"sandbox\t\tperceptron sandbox\n"
					"help\t\tprint this stuff\n"
					"exit\t\t...\n";
			}
			else if (first == "digits")
			{
				Digits digits;
				digits.Run();
			}
			else if (first == "sandbox")
			{
				Sandbox sandbox;
				sandbox.Run();
			}
			else if (first == "exit")
			{
				active = false;
			}
			else
			{
				std::cout << '\"' << tokens[0] << "\" not recognised\a\n";
			}
		}
	}

	System::CloseConsole();
	return 0;
}
