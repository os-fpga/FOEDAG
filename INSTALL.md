### Clone and Initialize Submodules
```
git clone https://github.com/os-fpga/FOEDAG.git
cd FOEDAG
git submodule update --init --recursive
```

### Compile source codes

```
  make
or
  make debug
or
  make release_no_tcmalloc (For no tcmalloc)
  
make install (/usr/local/bin and /usr/local/lib/foedag by default which requires sudo privilege,
             use PREFIX= for alternative locations.)
```

### Run quick test

```
make test
```

### Build documentation

```
make doc
```

You can view the documentation under the ``docs/build/html`` using a web browser, e.g., Firefox
