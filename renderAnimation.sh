set -e

FRAMES_DIR="${1:-frames}"
PREFIX="${2:-frame}"
FPS="${3:-24}"
FORMAT="${4:-gif}"

if [ ! -d "$FRAMES_DIR" ]; then
    echo "Error: directory '$FRAMES_DIR' does not exist."
    exit 1
fi

PATTERN="${FRAMES_DIR}/${PREFIX}_%04d.ppm"
OUTPUT="${PREFIX}_anim.${FORMAT}"

echo "Assembling frames from '${FRAMES_DIR}/${PREFIX}_*.ppm' at ${FPS} fps -> ${OUTPUT}"

if [ "$FORMAT" = "mp4" ]; then
    if ! command -v ffmpeg &>/dev/null; then
        echo "Error: ffmpeg not found. Install it with: sudo apt install ffmpeg"
        exit 1
    fi

    ffmpeg -y -framerate "$FPS" -i "$PATTERN" \
        -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" \
        -c:v libx264 -pix_fmt yuv420p \
        "$OUTPUT"

elif [ "$FORMAT" = "gif" ]; then
    if command -v ffmpeg &>/dev/null; then
        PALETTE=$(mktemp /tmp/palette_XXXXXX.png)
        ffmpeg -y -framerate "$FPS" -i "$PATTERN" \
            -vf "palettegen" "$PALETTE" 2>/dev/null
        ffmpeg -y -framerate "$FPS" -i "$PATTERN" \
            -i "$PALETTE" -lavfi "paletteuse" \
            "$OUTPUT"
        rm -f "$PALETTE"

    elif command -v convert &>/dev/null; then
        DELAY=$(echo "scale=0; 100 / $FPS" | bc)
        convert -delay "$DELAY" -loop 0 \
            "${FRAMES_DIR}/${PREFIX}_*.ppm" \
            "$OUTPUT"

    else
        echo "Error: neither ffmpeg nor ImageMagick (convert) found."
        echo "Install one of:"
        echo "sudo apt install ffmpeg"
        echo "sudo apt install imagemagick"
        exit 1
    fi

else
    echo "Error: unknown format '$FORMAT'. Use 'gif' or 'mp4'."
    exit 1
fi

echo "Done! Output: $OUTPUT"