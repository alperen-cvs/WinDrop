# WinDrop
Simple packet dropper for Windows 

* Windrop uses WinDivert so you need WinDivert .dll and .sys file
WinDrop uses Windivert, so you need the .dll and .sys files. These two files must be in the program's directory otherwise it will not work.

You must first save the sys file.

* sc create firewall binPath=\path\to\Driver.sys type=kernel
(Requires administrator rights)

If you want to compile it yourself:

* gcc main.c .\path\to\windivert.dll
-I include -lws2_32 -lgdi32 -luser32

Remember, this blocks all TCP and UDP packets and only works with IPv4.

NOTE: This program should not be used for any malicious purposes. You should also know that I will not be held responsible in any way if used.
