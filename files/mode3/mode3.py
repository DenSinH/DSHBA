import numpy as np
import png

vram = np.fromfile("VRAM.dump", dtype=np.uint16)[:240 * 160].reshape((160, 240))

with open("vram.png", "wb") as f:
    writer = png.Writer(240, 160, bitdepth=16)
    writer.write(f, vram)