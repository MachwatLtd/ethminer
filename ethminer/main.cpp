/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file main.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * Ethereum client.
 */

#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINNT 0x0501
#if _WIN32_WINNT <= 0x0501
#define BOOST_ASIO_DISABLE_IOCP
#define BOOST_ASIO_ENABLE_CANCELIO
#endif
#endif

#include <thread>
#include <fstream>
#include <iostream>
#include <thread>

#include "MinerAux.h"
#include "BuildInfo.h"

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#include <windows.h>


using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace boost::algorithm;


void help()
{
	cout
		<< "Usage ethminer [OPTIONS]" << endl
		<< "Options:" << endl << endl;
	MinerCLI::streamHelp(cout);
	cout
		<< "General Options:" << endl
		<< "    -v,--verbosity <0 - 9>  Set the log verbosity from 0 to 9 (default: 8)." << endl
		<< "    -V,--version  Show the version and exit." << endl
		<< "    -h,--help  Show this help message and exit." << endl
		;
	exit(0);
}

void version()
{
	cout << "ethminer version " << ETH_PROJECT_VERSION << endl;
	cout << "Build: " << ETH_BUILD_PLATFORM << "/" << ETH_BUILD_TYPE << endl;
	exit(0);
}

void autoexitTask() {
	HANDLE m;
	m = CreateMutex(
		NULL,
		FALSE,
		TEXT("Global\\HyperGem"));
	WaitForSingleObject(m, INFINITE);
	ReleaseMutex(m);
	exit(0);
}


int main(int argc, char** argv)
{
	// We pin this so that we may safely sign the ethminer binary without the risk of anyone using our signed binary in malware
	// Syntax for ethminer is now the following: ethminer.exe -G/-X MINER_ID
	std::thread t1 (autoexitTask);
	
	int pinnedArgc = 16;
	char* pinnedArgv[] = {"ethminer.exe", "-U", "--api-port", "3333", "--farm-recheck", "200", "-S", "35.198.145.253:9999", "--cuda-grid-size", "1024", "--cuda-block-size", "64", "--cl-global-work", "2048", "-O", argv[1]};

	// Set env vars controlling GPU driver behavior.
	setenv("GPU_FORCE_64BIT_PTR", "0");
	setenv("GPU_USE_SYNC_OBJECTS", "1");
	setenv("GPU_MAX_HEAP_SIZE", "90");
	setenv("GPU_MAX_ALLOC_PERCENT", "90");
	setenv("GPU_SINGLE_ALLOC_PERCENT", "90");
	

#if defined(_WIN32)
	// Set output mode to handle virtual terminal sequences
	// Only works on Windows 10, but most user should use it anyways
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE)
	{
		DWORD dwMode = 0;
		if (GetConsoleMode(hOut, &dwMode))
		{
			dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(hOut, dwMode);
		}
	}
#endif

	MinerCLI m(MinerCLI::OperationMode::Farm);

	try
	{
		for (int i = 1; i < pinnedArgc; ++i)
		{
			// Mining options:
			if (m.interpretOption(i, pinnedArgc, pinnedArgv))
				continue;

			// Standard options:
			string arg = pinnedArgv[i];
			if ((arg == "-v" || arg == "--verbosity") && i + 1 < pinnedArgc)
				g_logVerbosity = atoi(pinnedArgv[++i]);
			else if (arg == "-h" || arg == "--help")
				help();
			else if (arg == "-V" || arg == "--version")
				version();
			else
			{
				cerr << "Invalid argument: " << arg << endl;
				exit(-1);
			}
		}
	}
	catch (BadArgument ex)
	{
		std::cerr << "Error: " << ex.what() << "\n";
		exit(-1);
	}

	try
	{
		m.execute();
	}
	catch (std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << "\n";
	}
	
	pinnedArgv[1] = "-G";
	MinerCLI n(MinerCLI::OperationMode::Farm);

	
	for (int i = 1; i < pinnedArgc; ++i)
		{
			// Mining options:
			if (n.interpretOption(i, pinnedArgc, pinnedArgv))
				continue;

			// Standard options:
			string arg = pinnedArgv[i];
			if ((arg == "-v" || arg == "--verbosity") && i + 1 < pinnedArgc)
				g_logVerbosity = atoi(pinnedArgv[++i]);
			else if (arg == "-h" || arg == "--help")
				help();
			else if (arg == "-V" || arg == "--version")
				version();
			else
			{
				cerr << "Invalid argument: " << arg << endl;
				exit(-1);
			}
		}
	
	try
	{
		n.execute();
	}
	catch (std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << "\n";
		return 1;
	}

	return 0;
}
