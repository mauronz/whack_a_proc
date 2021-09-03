# whack-a-proc

whack-a-proc is a tool that allows to monitor an executable and dump any PE file it loads in its own memory or injects in other processes.
For more info: https://mauronz.github.io/whack-a-proc

Usage:  
```
monitor.exe [options] *target_command_line*
```

Example:
```bash
monitor.exe /level 1 /all notepad.exe mytextfile.txt
```

Options:

- /level [0,1,2]:
    Each level increases the number of APIs hooked,
    potentially leading to new findings, but at the cost of performances.
	- 0: Process and thread manipulation.
	   Hooked APIs: NtCreateThread, NtCreateThreadEx, NtResumeThread, NtCreateUserProcess
	- 1: Making memory executable.
	   Hooked APIs: NtProtectVirtualMemory
	- 2: Library loading.
	   Hooked APIs: LoadLibraryA, LoadLibraryW

- /protect:
    Set a hook on ZwMapViewOfSection to prevent a remapping of ntdll.dll.
    If ntdll.dll is being mapped, the output pointer to the mapped section (BaseAddress)
    is replaced with the currently mapped ntdll.dll (that is hooked).

- /all:
    Inject into newly created processes without asking for confirmation.

- /verbose:
	Print logs.