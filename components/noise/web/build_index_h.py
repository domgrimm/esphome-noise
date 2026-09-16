#!/usr/bin/env python3
import gzip
import os

WEB_DIR = os.path.dirname(os.path.abspath(__file__))
HTML_PATH = os.path.join(WEB_DIR, "index.html")
OUTPUT_H = os.path.join(os.path.dirname(WEB_DIR), "noise_dashboard_index.h")

def build():
    with open(HTML_PATH, "r", encoding="utf-8") as f:
        html = f.read()

    # Basic minification: remove empty lines
    lines = [line.strip() for line in html.splitlines() if line.strip()]
    minified = "\n".join(lines)

    compressed = gzip.compress(minified.encode("utf-8"), compresslevel=9)
    size = len(compressed)
    print(f"Original size: {len(html)} bytes")
    print(f"Minified size: {len(minified)} bytes")
    print(f"Gzipped size:  {size} bytes")

    hex_bytes = [f"0x{b:02x}" for b in compressed]
    wrapped_lines = []
    for i in range(0, len(hex_bytes), 16):
        wrapped_lines.append("  " + ", ".join(hex_bytes[i:i+16]))

    joined_bytes = ",\n".join(wrapped_lines)

    header_content = f"""#pragma once

#include "esphome/core/progmem.h"
#include <cstddef>
#include <cstdint>

namespace esphome::noise {{

constexpr uint8_t NOISE_INDEX_HTML_GZ[{size}] PROGMEM = {{
{joined_bytes}
}};

constexpr size_t NOISE_INDEX_HTML_GZ_SIZE = {size};

}}  // namespace esphome::noise
"""

    with open(OUTPUT_H, "w", encoding="utf-8") as f:
        f.write(header_content)

    print(f"Wrote {OUTPUT_H}")

if __name__ == "__main__":
    build()
