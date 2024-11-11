# Brightness Control

## Table of Contents
- [About](#about)
- [Build](#build)
- [Usage](#usage)

## About
This app can change your laptop screen brightness using RWEverything.exe. This repository is inspired by https://gist.github.com/Orochimarufan/04da79841395710c0dbc9d28a22e29f2

## Build
1. Clone the repository 
```bash
git clone https://github.com/XniceCraft/BrightnessControl
```
2. Create build dir
```bash
cd BrightnessControl
mkdir build
cd build
```
3. Create makefile
```bash
cmake -G "MinGW Makefiles ..
```
4. Build the project
```bash
mingw32-make.exe
```
5. Finish
```
There is an executable named BrightnessControl.exe that you can run
```

## Usage
### First Installation
1. Download RWEverything.exe from https://rweverything.com/download/
2. Run the application for once
```
Double click it
```
3. Open config.ini
```
Edit the RWPATH section and copy your RWEverything.exe full path
```
4. Save and re-run

### Normal usage
1. Run the app using double click or run it at startup
2. To exit the application, just right click on the system tray icon