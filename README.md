# FOEDAG <img src="./docs/source/overview/figures/osfpga_logo.png" width="200" align="right">

[![Documentation Status](https://readthedocs.org/projects/foedag/badge/?version=latest)](https://foedag.readthedocs.io/en/latest/?badge=latest)

FOEDAG denotes Qt-based Framework Open EDA Gui

## Documentation

FOEDAG's [full documentation](https://foedag.readthedocs.io/en/latest/) includes tutorials, tool options and contributor guidelines.

## Build instructions

Read [`INSTALL`](INSTALL.md) for more details

```bash
  make
or
  make debug
or
  make release_no_tcmalloc (For no tcmalloc)
make install (/usr/local/bin and /usr/local/lib/foedag by default which requires sudo privilege, use DESTDIR= for alternative locations. Note: do not use PREFIX=)
```
