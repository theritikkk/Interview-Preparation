"""Write synthetic sample photos to data/sample/ so you can try the CLI before using a real camera."""
import os, numpy as np, cv2
from gauge import *
os.makedirs("data/sample/cal", exist_ok=True); r = np.random.RandomState(7)
for i in range(14): rv, tv = random_board_pose(r, 35); cv2.imwrite(f"data/sample/cal/cal_{i:02d}.png", render(rv, tv, "board", seed=900 + i))
rv, tv = measurement_pose(np.random.RandomState(8), jitter=True); cv2.imwrite("data/sample/part.png", render(rv, tv, "measure", seed=950))
print("wrote data/sample/cal/*.png (14 images) and data/sample/part.png")
