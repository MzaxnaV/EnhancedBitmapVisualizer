# Enhanced Bitmap Visualizer Plugin

A visualizer plugin for ImHex to visualise bitmaps, based on the official [ImHex](https://github.com/WerWolv/ImHex) plugin template.

## Usage
- For now, download the plugin from the github actions actifact. 
- Pass a swizzle `order` to it like this: `[[hex::visualize("custom_bitmap", pattern, width, height, 0x01020304)]]` where R -> 01, G -> 02, B -> 03, A -> 04.

## Notes:
- `.github/workflows`: The CI script is based on an older imhex version (1.32.2v).
