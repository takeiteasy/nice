#!/usr/bin/env python
from PIL import Image
import sys, os, argparse

def die(msg="Unknown error!"):
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(1)

def explode(image_path, tile_width, tile_height, padding, out_path):
    img = None
    try:
        img = Image.open(image_path)
    except FileNotFoundError as e:
        die(str(e))

    width, height = img.size
    if tile_width > width or tile_height > height:
        die(f"Tile size larger than image: tile size: {tile_width},{tile_height}, image size: {width},{height}")
    if width % tile_width != 0 or height % tile_height != 0:
        die(f"Tile size is not multiple of image size: tile size: {tile_width},{tile_height}, image size: {width},{height}")

    rows = height // tile_height
    cols = width // tile_width
    new_width = (cols * tile_width) + ((cols + 1) * padding)
    new_height = (rows * tile_height) + ((rows + 1) * padding)
    outimg = Image.new(size=(new_width, new_height), mode="RGBA")
    for y in range(rows):
        for x in range(cols):
            rect = (x * tile_width, y * tile_height, (x + 1) * tile_width, (y + 1) * tile_height)
            clip = img.crop(rect)
            dx = x * tile_width + ((x + 1) * padding)
            dy = y * tile_height + ((y + 1) * padding)
            dst = (dx, dy, dx+clip.width, dy+clip.height)
            try:
                outimg.paste(clip, dst)
            except ValueError as e:
                print(dst)
                die(str(e))

    outimg.save(out_path)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Split image into smaller tiles")
    parser.add_argument("image", help="Path to the image file")
    parser.add_argument("--width", type=int, default=8, help="Width of each tile (default: 8)")
    parser.add_argument("--height", type=int, default=8, help="Height of each tile (default: 8)")
    parser.add_argument("--padding", type=int, default=4, help="Padding to add around tiles (default: 5)")
    parser.add_argument("--out", default=None, help="Output path (default: [old path].padding.[old extension])")
    args = parser.parse_args()
    if not args.out:
        p = args.image.split(".")
        args.out = ".".join(p[:-1]) + ".padded." + p[-1]
    explode(args.image, args.width, args.height, args.padding, args.out)
