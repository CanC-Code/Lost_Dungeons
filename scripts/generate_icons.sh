#!/bin/bash
set -e

echo "Starting advanced icon generation using ImageMagick..."

# Define the base resources directory
RES_DIR="app/src/main/res"

# Create the mipmap directories
mkdir -p "$RES_DIR/mipmap-mdpi"
mkdir -p "$RES_DIR/mipmap-hdpi"
mkdir -p "$RES_DIR/mipmap-xhdpi"
mkdir -p "$RES_DIR/mipmap-xxhdpi"
mkdir -p "$RES_DIR/mipmap-xxxhdpi"

# Generate a high-resolution base icon (512x512)
# The drawing order is critical: Back to Front
# 1. Sky Gradient (Dark blue/indigo)
# 2. Moon (Pale yellow)
# 3. Left Mountain + Snowcap
# 4. Right Mountain + Snowcap
# 5. Cavern Mound (Dark stone)
# 6. Left Spruce Tree (3 tiers)
# 7. Right Spruce Tree (3 tiers)
# 8. Cavern Entrance (Pure black)
# 9. Teal Stalagmites / Mystic Glow

convert -size 512x512 gradient:"#111827"-"#312e81" \
  -fill "#fef08a" -draw "circle 400,100 400,135" \
  -fill "#475569" -draw "polygon -50,400 150,120 350,400" \
  -fill "#f8fafc" -draw "polygon 150,120 100,190 125,210 150,180 175,210 200,190" \
  -fill "#334155" -draw "polygon 100,400 350,70 600,400" \
  -fill "#e2e8f0" -draw "polygon 350,70 300,140 325,160 350,130 375,160 400,140" \
  -fill "#1c1917" -draw "ellipse 256,400 240,200 0,360" \
  -fill "#064e3b" -draw "polygon 70,180 20,290 120,290" \
  -fill "#064e3b" -draw "polygon 70,240 10,360 130,360" \
  -fill "#064e3b" -draw "polygon 70,300 0,440 140,440" \
  -fill "#022c22" -draw "polygon 440,160 390,270 490,270" \
  -fill "#022c22" -draw "polygon 440,220 380,340 500,340" \
  -fill "#022c22" -draw "polygon 440,280 370,420 510,420" \
  -fill "#000000" -draw "ellipse 256,450 140,180 0,360" \
  -fill "#0891b2" -draw "polygon 200,512 220,380 240,512" \
  -fill "#06b6d4" -draw "polygon 240,512 260,340 280,512" \
  -fill "#22d3ee" -draw "polygon 280,512 300,400 320,512" \
  base_icon.png

echo "Base vector art generated. Resizing for Android densities..."

# Resize and output to the respective mipmap folders
convert base_icon.png -resize 48x48 "$RES_DIR/mipmap-mdpi/ic_launcher.png"
convert base_icon.png -resize 72x72 "$RES_DIR/mipmap-hdpi/ic_launcher.png"
convert base_icon.png -resize 96x96 "$RES_DIR/mipmap-xhdpi/ic_launcher.png"
convert base_icon.png -resize 144x144 "$RES_DIR/mipmap-xxhdpi/ic_launcher.png"
convert base_icon.png -resize 192x192 "$RES_DIR/mipmap-xxxhdpi/ic_launcher.png"

# Clean up the temporary base image
rm base_icon.png

echo "Icon generation and placement complete."
