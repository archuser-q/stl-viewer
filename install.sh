#!/usr/bin/env bash
# Build and install STL Viewer for the current Ubuntu user.
# Run from the project root: ./install.sh
set -euo pipefail

cd "$(dirname "$(readlink -f "$0")")"

if [[ ! -f CMakeLists.txt ]]; then
    echo "Error: put install.sh in the project root." >&2
    exit 1
fi

if [[ ! -f icon.png ]]; then
    echo "Error: icon.png not found." >&2
    exit 1
fi

sudo apt update
sudo apt install -y build-essential cmake git libgl1-mesa-dev xorg-dev \
    libwayland-dev libxkbcommon-dev libgtk-3-dev desktop-file-utils

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

mkdir -p "$HOME/.local/bin" "$HOME/.local/share/applications" "$HOME/.local/share/icons"
install -m 755 build/stl-viewer "$HOME/.local/bin/stl-viewer"
install -m 644 icon.png "$HOME/.local/share/icons/stl-viewer.png"

cat > "$HOME/.local/share/applications/stl-viewer.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=STL Viewer
Comment=View STL 3D models
Exec=$HOME/.local/bin/stl-viewer %f
Icon=$HOME/.local/share/icons/stl-viewer.png
Terminal=false
Categories=Graphics;Viewer;
EOF

chmod +x "$HOME/.local/share/applications/stl-viewer.desktop"
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
touch "$HOME/.local/share/applications/stl-viewer.desktop"
touch "$HOME/.local/share/applications"

echo "STL Viewer installed. Press Super and search for STL Viewer."