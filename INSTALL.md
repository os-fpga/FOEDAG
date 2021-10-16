### Clone and Initialize Submodules



```
On Ubuntu 18.04:
   sudo apt-get install qt5-default

On Ubuntu 21.04:
   sudo apt-get install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools
```

```
git clone https://github.com/os-fpga/FOEDAG.git
cd FOEDAG
git submodule update --init --recursive
```

### Compile source codes

```
make
```

### Run quick test

```
make test
```
