#!/bin/bash
set -e

echo "Starting icon generation using ImageMagick..."

# Define the base resources directory
RES_DIR="app/src/main/res"

# Create the mipmap directories
mkdir -p "$RES_DIR/mipmap-mdpi"
mkdir -p "$RES_DIR/mipmap-hdpi"
mkdir -p "$RES_DIR/mipmap-xhdpi"
mkdir -p "$RES_DIR/mipmap-xxhdpi"
mkdir -p "$RES_DIR/mipmap-xxxhdpi"

# Generate a high-resolution base icon (512x512)
# Design: Dark cavern gradient background with a glowing teal crystal/portal shape
convert -size 512x512 radial-gradient:"#2C3E50"-"#05050A" \
  -fill "#1ABC9C" -draw "polygon 256,120 160,420 352,420" \
  -fill "#16A085" -draw "polygon 256,180 190,420 322,420" \
  -fill "#0E6655" -draw "polygon 256,250 220,420 292,420" \
  base_icon.png

echo "Base icon generated. Resizing for Android densities..."

# Resize and output to the respective mipmap folders
convert base_icon.png -resize 48x48 "$RES_DIR/mipmap-mdpi/ic_launcher.png"
convert base_icon.png -resize 72x72 "$RES_DIR/mipmap-hdpi/ic_launcher.png"
convert base_icon.png -resize 96x96 "$RES_DIR/mipmap-xhdpi/ic_launcher.png"
convert base_icon.png -resize 144x144 "$RES_DIR/mipmap-xxhdpi/ic_launcher.png"
convert base_icon.png -resize 192x192 "$RES_DIR/mipmap-xxxhdpi/ic_launcher.png"

# Clean up the temporary base image
rm base_icon.png

echo "Icon generation and placement complete."
