# Installing Liminal Rack on a Mac

For now only providing instructions for downloading Liminal source code and compiling it. 
And only instructions for a Mac are provided since that is all I use. Sorry!

## VCV Rack official Instructions
In case the instructions for running VCV Rack are useful, they can be found at 
https://vcvrack.com/manual/Building

## Setup on Raspberry Pi OS

Need some utilities, like cmake and jq.
```
sudo apt install -y cmake
sudo apt install -y jq
```

## Setup on MacOS

First, need the tool brew to install other things. In terminal window simply use:
```
$ /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Second, need to make sure certain tools are available on your computer. 
Set up development environment and add dependencies by using brew:
```
$ brew install git wget cmake autoconf automake libtool jq python zstd pkg-config
This can update quite a bit and take a couple of minutes.
```

## Download Liminal Rack software

Download Liminal Rack source code from the skibu github repo:
```
$ mkdir ~/vscode-projects
$ cd ~/vscode-projects
$ git clone --recurse-submodules https://github.com/skibu/LiminalRack
$ cd LiminalRack
```
Note: original Rack is at https://github.com/VCVRack/Rack.git, but these instructions are for Liminal Rack.

If happen to not use --recurse-submodules when cloning then need to clone submodules separately. Also good for updating changes:
`$ git submodule update --init --recursive`

## Downloading source code for Fundamental module 
Need to get the Fundamental Rack plugins. 
These modules are the standard mostly utility ones. They are very useful and so should be used. 
Need to download the Fundamental source code, compile it, and then place the compiled library in your Rack2
user data directory, which for a Mac is at `~Library/Application\ Support/Rack2/plugins-COMPUTER-TYPE/Fundam`

Need to use the skibu fork of Fundamental since had to modify it to work with changes made in Liminal Rack.
Can also see the VCV Rack instructions at https://vcvrack.com/manual/Building#Building-Rack-plugins if more info needed.

```
$ cd ~/vscode-projects/LiminalRack/plugins
$ git clone https://github.com/skibu/Fundamental
$ cd Fundamental
$ git submodule update --init --recursive
$ make dep
$ make        # compiles
$ make dist   # puts library into proper place
$ # make install # which does both make and make dist
```

Note: if you are going to use VSCode to deal with the Fundamental code then you 
should open up a separate VSCode project window 
for Fundamental since it is a separate git repo.

## Building Liminal Rack
You can build Liminal Rack in VSCode or via command line. Doing it in VSCode is 
best if you are going to develop the code further. But if you just want to run
the existing code then the following command line instructions are easiest.
```
$ cd ~/vscode-projects/LiminalRack/
$ make dep     # Build dependencies (can take 5-50 minutes!)
$ make         # Build Rack (may take 1-5 minutes)
```

## Run Liminal Rack:
```
$ Rack &
```

And you should get the Liminal version of Rack!
