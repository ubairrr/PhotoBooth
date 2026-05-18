#to change bootup animation later, run this command: 
# first convert to mjpeg
# ffmpeg -i boot_anim.mp4 -q:v 3 -vf "scale=240:320" boot_anim.mjpeg
# then convert to c array
# python3 mjpeg2c.py boot_anim.mjpeg src/assets/boot_anim_data.h

import sys
import os

def convert(input_file, output_file):
    with open(input_file, "rb") as f:
        data = f.read()
    
    with open(output_file, "w") as f:
        f.write("#pragma once\n")
        f.write("#include <stdint.h>\n")
        f.write("#include <pgmspace.h>\n\n")
        f.write(f"const size_t boot_anim_len = {len(data)};\n")
        f.write("const uint8_t boot_anim_data[] PROGMEM = {\n")
        
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
            f.write(f"    {hex_str}")
            if i + 16 < len(data):
                f.write(",")
            f.write("\n")
            
        f.write("};\n")

if __name__ == "__main__":
    convert(sys.argv[1], sys.argv[2])
