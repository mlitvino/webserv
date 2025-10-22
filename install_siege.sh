#!/bin/bash

set -e  # Exit on error
set -o pipefail

# Define install paths
SRC_DIR="$HOME/src"
INSTALL_PREFIX="$HOME/.local"
PATH_UPDATE_LINE='export PATH="$HOME/.local/bin:$PATH"'

echo "🔧 Creating source directory at $SRC_DIR..."
mkdir -p "$SRC_DIR"
cd "$SRC_DIR"

echo "🌐 Downloading latest Siege release..."
curl -LO https://download.joedog.org/siege/siege-latest.tar.gz

echo "📦 Extracting Siege..."
tar -xzf siege-latest.tar.gz

cd siege-*/

echo "⚙️ Configuring build with prefix $INSTALL_PREFIX..."
./configure --prefix="$INSTALL_PREFIX"

echo "🔨 Building Siege..."
make -j"$(nproc)"

echo "📥 Installing Siege to $INSTALL_PREFIX..."
make install

# Add ~/.local/bin to PATH in .zshrc if not already added
if ! grep -Fxq "$PATH_UPDATE_LINE" "$HOME/.zshrc"; then
    echo "➕ Adding $INSTALL_PREFIX/bin to PATH in .zshrc..."
    echo "$PATH_UPDATE_LINE" >> "$HOME/.zshrc"
else
    echo "✅ PATH already set in .zshrc"
fi

# Update current session PATH
export PATH="$INSTALL_PREFIX/bin:$PATH"
echo "✅ PATH updated for current session."

# Test the install
echo "🧪 Verifying Siege installation..."
which siege
siege --version

echo "✅ Siege installation complete!"

