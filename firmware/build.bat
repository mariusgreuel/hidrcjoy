@echo off
setlocal EnableExtensions EnableDelayedExpansion

where /q avr-g++
if ERRORLEVEL 1 (
	if not defined AVR8_GNU_TOOLCHAIN (
		echo Environment variable AVR8_GNU_TOOLCHAIN is not set.
		exit /b 1
	)

	set PATH=!PATH!;!AVR8_GNU_TOOLCHAIN!\bin
)

where /q make
if ERRORLEVEL 1 (
	echo Make utility not found in PATH.
	echo Please install GNU Make and add it to your PATH.
	exit /b 1
)

make %*
