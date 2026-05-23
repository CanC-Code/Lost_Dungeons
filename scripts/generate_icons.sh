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
# Design: Dark stone background, deep black cave entrance, glowing teal stalagmites
convert -size 512x512 radial-gradient:"#303030"-"#101010" \
  -fill "#050505" -draw "ellipse 256,350 200,280 0,360" \
  -fill "#00E5FF" -draw "polygon 256,150 220,400 292,400" \
  -fill "#00B8D4" -draw "polygon 256,220 160,420 230,420" \
  -fill "#0091EA" -draw "polygon 256,260 280,420 360,420" \
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
