#pragma once

#include <iostream>
#include <Windows.h>
#include <chrono>
#include <TlHelp32.h>
#include <filesystem>
#include <thread>

#define TARGET "ReadyOrNotSteam-Win64-Shipping.exe"
#define LIBRARY "version.dll"

using namespace std::chrono_literals;