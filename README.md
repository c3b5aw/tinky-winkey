# tinky-winkey

Tinky winkey is an introduction to WINAPI and Windows Services.

The goal of the project is to wrap a keylogger within a Windows Service.

The `keylogger` must impersonate SYSTEM tokens and run in the background.

It is a good introduction and Windows services are pretty well documented all over the internet.

## Build

```bash
nmake all
	  re
```

## Usage

```bash
.\bin\svc.exe install
.\bin\svc.exe start
.\bin\svc.exe stop
.\bin\svc.exe delete
```

## Ressources

- VirtualBox
- Process Hacker 2
- Visual Studio
- Windows 11 Entreprise Evaluation Version 21H2