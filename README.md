<!-- README.md file --> 
# Description

The **tkLayout** represents a tracker geometry modelling tool and tracker performance analysis tool, which has been fully developed within 
the CMS collaboration at CERN. Its main aim is to study performance of a new silicon tracker, replacing the current one in a view of the 
Phase-2 upgrade (early 2020's). Besides that, the tkLayout is generic enough to be used for tracker studies at other experiments.

A tracker geometry is generated from simple configuration files (cf. GeometryManager & MainConfigHandler), defining the module types, 
tracker layout and material composition. Various support structures are then automatically added and services routed to build a truly 
realistic tracking system. The tkLayout has been programmed with flexibility in mind to provide natural support for various detector 
geometries and wide spectrum of analyses to be run with given layout and study the best trade-off among many figures of merit, 
such as: tracking resolution, material budget, power dissipation, cost etc. The tkLayout software is easy to be used thanks to its modular 
structure. The individual analysis modules, so-called analyzers, are run sequentially by so-called AnalysisManager, first to analyze the 
data and then to visualize the obtained results in compact & user-friendly way. The visualization is provided via html format. Several 
web pages are generated and included on the main web-site. They may contain various explanation texts, pictures, graphs, tables etc. 
(cf. RootWSite, RootWPage). Several important analyzers to be explicite mentioned: AnalyzerResolution (track resolution studies using the 
V.Karimaki approach for track fitting, see V.Karimaki - Effective circle fitting for particle trajectories), AnalyzerGeometry & AnalyzerMatBudget.   

Developed at CERN by Giovanni Bianchi, Nicoletta De Maio, Stefano Martina and Stefano Mersi.

# Getting the code

The code is accessible from an official github repository: https://github.com/tkLayout/tkLayout/tree/masterLite (use clone command to make a local copy):

    git clone -b masterLite https://github.com/tkLayout/tkLayout.git
    cd tkLayout

# Before the compilation/run

One needs several libraries to link the tkLayout with: a working version of ROOT (root and root-config should be in user's path) and a few
BOOST libraries:
  * boost_filesystem
  * boost_regex
  * the ROOT library set (follow instructions on https://root.cern.ch)

On Lxplus machine, one needs to run a bash shell and source the following configuration file:

    source setup_slc6.sh

# Compilation/Install

Compilation using cMake:

    mkdir build     (all object files, help files, libs, ... will be kept here)
    cd build
    cmake ..        (generate makefile)
    make install    (or write make all + make install)
    make doc        (generate Doxygen-based documentation in doc directory)

    make uninstall  (if cleaning needed)
    rm *            (clean all content in build directory & restart if needed)

Create first a **build** directory in the tkLayout home directory. All make related content will be then created here. Change the working 
directory to **build** and call **cmake ..** command (directing cMake to the tkLayout uppermost directory such as it reads correctly the 
CMakeLists.txt configuration file). After all MakeFiles are generated by cmake, call **make all** (**make install**) to compile tkLayout. 
Optional **install** copies the executables into tkLayout/bin directory and creates symlinks in ${HOME}/bin directory. To revert this 
procedure, apply **uninstall** option. 

## Documentation
Doxygen documentation is generated using **doc** option and all the generated html files are saved in tkLayout/doc/html directory.

## First-time install
If this is the first time that you install tkLayout, a few questions will be asked and a tkLayout configuration
file will be created in $HOME/.tkgeometry:

1. One needs to provide the destination directory for the www output of the program (proabably
  something like /afs/cern.ch/user/y/yourname/www/layouts) this directory needs to be writable by you
  and readable by the web server.
     The style files will be copied during the installation to that directory (for example
  /afs/cern.ch/user/y/yourname/www/layouts/style ). If the style files get changed during the development
  a new ./install.sh will be needed to get this propagated to the output.
     Alternatively, one can choose to make a symbolic link between the source directory style directory and
  the layout directory. This avoids the need of repeating the ./install.sh at every update of the style files
  but in order to do this the source directory should also be within the reach of the web server.
2. The set of momentum values to be used for the simulation
3. Project name
4. Author of generated results

# Update
To get the latest development version (usually UNSTABLE) one simply types

    git fetch
    git chechout masterLite (or devLite)
    make
    make install
  
