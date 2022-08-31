# Install required dependencies for Ubuntu systems
sudo apt-get update -qq
sudo apt install -y \
  build-essential \
  python3 \
  ninja-build \
  libevent-dev \
  libjson-c-dev \
  flex \
  bison \
  libfl-dev \
  libfl2 \
  verilator \
  zlibc \
  zlib1g-dev

# Install required Python3 packages.
pip3 install \
  setuptools \
  requests \
  pexpect \
  meson


# Download/Install LiteX.
wget https://raw.githubusercontent.com/enjoy-digital/litex/master/litex_setup.py
chmod +x litex_setup.py
./litex_setup.py --init
sudo ./litex_setup.py --install

# List installed python packages
pip list

# Download/Install RISC-V GCC toolchain.
./litex_setup.py --gcc=riscv
sudo mkdir /usr/local/riscv
if [ -d "/usr/local/riscv" ] 
then
    echo "Directory /usr/local/riscv exists." 
else
    echo "Error: Directory /usr/local/riscv does not exists."
fi
ls -alh
echo "Parent Dir:"
ls -la ..
echo "Check if .gitignore exists: ls .gitignore"
ls -la .gitignore
if [ -f .gitignore ]; then
    echo ".gitignore preset, checking parent directory for rscv64 files"
    sudo cp -r ../riscv64-*/* /usr/local/riscv
else
    echo "no .gitignore preset, checking current directory for rscv64 files"
    sudo cp -r riscv64-*/* /usr/local/riscv
fi
