#!/usr/bin/env bash
#
# bootstrap_dev_env.sh
# Installs dependencies for Verilator + AFL++ hardware fuzzing environment.
# - Creates and uses a Python virtualenv (.venv) in the project directory
# - Installs system packages (Ubuntu/Debian)
# - Installs Verilator (apt or build-from-source)
# - Installs AFL++ (apt or build-from-source)
# - Optionally builds riscv toolchain & spike if INSTALL_RISCV=1
#
# Usage:
#   ./bootstrap_dev_env.sh                # run as regular user (preferred)
#   sudo INSTALL_RISCV=1 ./bootstrap_dev_env.sh  # will still work but venv will be created as root
#
set -euo pipefail
trap 'rc=$?; echo -e "\n[ERROR] Failed at line $LINENO: $BASH_COMMAND (exit $rc)"; exit $rc' ERR

# ---------- User config ----------
INSTALL_RISCV="${INSTALL_RISCV:-0}"               # set to 1 to build riscv-gnu-toolchain + spike (slow)
VERILATOR_VER="${VERILATOR_VER:-4.248}"           # if building verilator from source, preferred tag
AFLPP_REPO="${AFLPP_REPO:-https://github.com/AFLplusplus/AFLplusplus.git}"
RISCV_TOOLCHAIN_REPO="${RISCV_TOOLCHAIN_REPO:-https://github.com/riscv/riscv-gnu-toolchain.git}"
SPIKE_REPO="${SPIKE_REPO:-https://github.com/riscv/riscv-isa-sim.git}"
PREFIX="${PREFIX:-/usr/local}"                     # install prefix for built tools
VENV_DIR="${VENV_DIR:-.venv}"                     # virtualenv directory (in project)
MAKEFLAGS="${MAKEFLAGS:--j$(nproc 2>/dev/null || echo 2)}"

# ---------- helpers ----------
info() { echo -e "\033[1;34m[INFO]\033[0m $*"; }
warn() { echo -e "\033[1;33m[WARN]\033[0m $*"; }
err()  { echo -e "\033[1;31m[ERROR]\033[0m $*" >&2; exit 1; }

detect_os() {
  if [ "$(uname -s)" = "Darwin" ]; then
    echo "macos"
  elif [ -f /etc/os-release ]; then
    . /etc/os-release
    echo "${ID}"
  else
    uname -s
  fi
}

ensure_python() {
  if ! command -v python3 >/dev/null 2>&1; then
    err "python3 not found. Please install python3."
  fi
}

# ---------- Python venv ----------
setup_venv() {
  ensure_python
  # If script run with sudo and SUDO_USER available, create venv in current directory but owned by root.
  if [ -d "$VENV_DIR" ]; then
    info "Virtualenv already exists at $VENV_DIR"
  else
    info "Creating Python virtualenv at $VENV_DIR"
    python3 -m venv "$VENV_DIR"
    info "Virtualenv created."
  fi
  # Use venv's python/pip for installs
  PIP="$PWD/$VENV_DIR/bin/pip"
  PY="$PWD/$VENV_DIR/bin/python"

  if [ ! -x "$PIP" ]; then
    err "pip not found in venv ($PIP). Try 'python3 -m venv $VENV_DIR' manually."
  fi

  info "Upgrading pip inside venv"
  "$PY" -m pip install --upgrade pip setuptools wheel >/dev/null

  info "Installing Python tooling in venv: pyelftools, tabulate"
  "$PIP" install pyelftools tabulate >/dev/null
  info "Python packages installed in $VENV_DIR"
}

# ---------- platform-specific installs (Ubuntu/Debian) ----------
install_debian_packages() {
  info "Updating apt and installing required Debian packages"
  sudo apt-get update -y
  sudo apt-get install -y build-essential git curl wget cmake pkg-config \
       autoconf automake libtool bison flex python3-venv python3-pip \
       g++ make libfl-dev libglib2.0-dev libelf-dev libexpat1-dev \
       libreadline-dev libncurses5-dev zlib1g-dev ca-certificates \
       texinfo patchutils xz-utils
  # optional helpful tools
  sudo apt-get install -y llvm clang lld || true
  info "System packages installed."
}

# ---------- Verilator ----------
install_verilator_pkg_or_build() {
  if command -v verilator >/dev/null 2>&1; then
    info "Verilator already installed: $(verilator --version 2>/dev/null | head -n1)"
    return 0
  fi

  info "Attempting to install verilator from apt..."
  if sudo apt-get install -y verilator; then
    info "Verilator installed via apt."
    return 0
  fi

  warn "Verilator apt package not available or failed - building Verilator from source (this can take several minutes)."
  TMPDIR="$(mktemp -d)"
  cd "$TMPDIR"
  if git ls-remote --exit-code --heads https://github.com/verilator/verilator.git "refs/heads/master" >/dev/null 2>&1; then
    git clone --depth 1 --branch "v${VERILATOR_VER}" https://github.com/verilator/verilator.git || git clone https://github.com/verilator/verilator.git
  else
    git clone https://github.com/verilator/verilator.git
  fi
  cd verilator
  autoconf
  ./configure --prefix="$PREFIX"
  make $MAKEFLAGS
  sudo make install
  info "Verilator built & installed to $PREFIX."
  cd - >/dev/null
  rm -rf "$TMPDIR"
}

# ---------- AFL++ ----------
install_aflplusplus() {
  if command -v afl-fuzz >/dev/null 2>&1; then
    info "AFL++ already installed: $(afl-fuzz --version 2>/dev/null | head -n1 || true)"
    return 0
  fi

  info "Attempting to install AFL++ from apt..."
  if sudo apt-get install -y afl++; then
    info "AFL++ installed via apt."
    return 0
  fi

  warn "AFL++ apt package not available or failed - building AFL++ from source."
  TMP="$(mktemp -d)"
  git clone --depth 1 "$AFLPP_REPO" "$TMP/AFLplusplus"
  cd "$TMP/AFLplusplus"
  make distrib $MAKEFLAGS || make $MAKEFLAGS
  sudo make install
  info "AFL++ built and installed."
  cd - >/dev/null
  rm -rf "$TMP"
}

# ---------- Optional: RISC-V toolchain & Spike ----------
install_riscv_toolchain_and_spike() {
  info "Installing RISC-V toolchain and Spike (this is optional and may be slow)."
  sudo apt-get update -y
  sudo apt-get install -y autoconf automake libmpc-dev libmpfr-dev libgmp-dev gawk \
    build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev

  BUILD_DIR="${HOME}/riscv-tools"
  mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  if [ ! -d "riscv-gnu-toolchain" ]; then
    info "Cloning riscv-gnu-toolchain (this may take a while)..."
    git clone --depth 1 "$RISCV_TOOLCHAIN_REPO"
  fi
  cd riscv-gnu-toolchain
  info "Building riscv-gnu-toolchain (may take long)"
  ./configure --prefix="$PREFIX/riscv"
  make $MAKEFLAGS
  export PATH="$PREFIX/riscv/bin:$PATH"
  info "riscv toolchain installed to $PREFIX/riscv."

  cd "$BUILD_DIR"
  if [ ! -d "riscv-isa-sim" ]; then
    info "Cloning spike (riscv-isa-sim)..."
    git clone --depth 1 "$SPIKE_REPO"
  fi
  cd riscv-isa-sim
  mkdir -p build && cd build
  cmake -DCMAKE_INSTALL_PREFIX="$PREFIX" ..
  make $MAKEFLAGS
  sudo make install
  info "Spike installed to $PREFIX/bin."
  cd - >/dev/null
}

# ---------- main ----------
main() {
  osid=$(detect_os)
  info "Detected OS: $osid"
  ensure_python

  # On non-Debian systems we won't attempt apt; script supports Debian/Ubuntu only for system installs
  if [ "$osid" != "ubuntu" ] && [ "$osid" != "debian" ]; then
    warn "This script was designed for Ubuntu/Debian. On macOS or other OSes, please adapt the package manager commands (Homebrew on macOS)."
  fi

  # Create and use venv
  setup_venv

  # Install system packages (Debian)
  if [ "$osid" = "ubuntu" ] || [ "$osid" = "debian" ]; then
    install_debian_packages
  else
    warn "Skipping apt installs (unsupported OS); please install required packages manually."
  fi

  # Verilator
  install_verilator_pkg_or_build

  # AFL++
  install_aflplusplus

  # Optional: riscv toolchain & spike
  if [ "${INSTALL_RISCV}" = "1" ] || [ "${INSTALL_RISCV}" = "true" ]; then
    install_riscv_toolchain_and_spike
  else
    info "Skipping riscv toolchain & spike. Set INSTALL_RISCV=1 to enable."
  fi

  # Final checks
  info "Installation summary (commands below may be missing if not installed):"
  if command -v verilator >/dev/null 2>&1; then
    info "verilator: $(verilator --version 2>/dev/null | head -n1)"
  else
    warn "verilator: NOT FOUND"
  fi
  if command -v afl-fuzz >/dev/null 2>&1; then
    info "afl-fuzz: $(afl-fuzz --version 2>/dev/null | head -n1 || true)"
  else
    warn "afl-fuzz: NOT FOUND"
  fi
  if [ -x "$PWD/$VENV_DIR/bin/python" ]; then
    info "python in venv: $($PWD/$VENV_DIR/bin/python --version 2>&1)"
    info "pip in venv: $($PWD/$VENV_DIR/bin/pip --version 2>&1)"
  else
    warn "Local venv python not found at $PWD/$VENV_DIR/bin/python"
  fi

  echo
  info "Done. Quick next steps (run from your project root):"
  cat <<'EOF'
  1) Activate the venv when you work on the project:
       source .venv/bin/activate

  2) Build your Verilator simulation example (adapt to your project):
       ./tools/build.sh top

  3) Run a single test case (example):
       ./tools/run_once.sh ./obj_dir/Vtop seeds/seed_empty.bin

  4) Start AFL++ fuzzing:
       ./tools/run_fuzz.sh ./obj_dir/Vtop

  Note: If you used sudo to run this script, the venv is owned by root.
        Prefer running this script as a regular user so the venv is created for you.
EOF
}

# run
main "$@"
