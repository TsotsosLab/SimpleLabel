# SimpleLabel

SimpleLabel is a image labelling tool designed to simplify the task of labelling
targets (or regions of interest) on image sequences while providing simple and
intuitive user interface. At the moment SimpleLabel supports only rectangular regions 
and uses linear interpolation to create intermediate marked regions.

Developed  by Eugene Simine at York University, Toronto in Laboratory for Active and 
Attentive Vision (http://www.cse.yorku.ca/LAAV/home/) headed by John K. Tsotsos.

## Contact:
- Eugene Simine <eugene@cse.yorku.ca> or
- John K. Tsotsos <tsotsos@cse.yorku.ca>

## Installation:
Windows binaries can be obtained from:
https://github.com/TsotsosLab/SimpleLabel/releases/download/0.8.0/SimpleLabel-0.8.0.zip

Unzip the file and run `SimpleLabel.exe`. Note that it requires [Microsoft Visual C++ 2010 SP1 Redistributable Package (x86)](http://www.microsoft.com/download/en/details.aspx?id=8328) to be installed.

### Windows:
Run `win_config.bat`. That will create MSVS project.

In case you get a compilation error, try the following:

1. Open the SimpleLabel project
2. Go to: `Project->Properties->Configuration Properties->General->Target Name`
3. There will be a number appended to value, should look something like this "SimpleLabel1"
4. Delete that number. The Target Name should have value "SimpleLabel"
5. Click OK
6. Compile the project

### Linux:
```
$ qmake SimpleLabel.pro
$ make
$ ./SimpleLabel
```

## Development

Development is mainly done on Windows 7, MS VisualStudio 2010. Major releases are tested on Ubuntu.

### Requirements

Software:
- Qt4.*
- OpenCV2.4.2
- [Microsoft Visual C++ 2010 SP1 Redistributable Package (x86)](http://www.microsoft.com/download/en/details.aspx?id=8328)

OS Support:
- Windows (XP, Vista, 7)
- Linux (Ubuntu 10.04)
- MacOS (in theory)
