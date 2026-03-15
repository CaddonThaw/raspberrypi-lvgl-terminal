import sys

import cv2
import numpy as np


class LvWindow:
    def __init__(self, width=160, height=128):
        self.width = width
        self.height = height
        self.stream = sys.stdout.buffer

    def show(self, frame):
        if frame is None:
            raise RuntimeError("Frame is empty")

        resized = cv2.resize(frame, (self.width, self.height), interpolation=cv2.INTER_LINEAR)
        rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)

        red = (rgb[:, :, 0].astype(np.uint16) >> 3) << 11
        green = (rgb[:, :, 1].astype(np.uint16) >> 2) << 5
        blue = rgb[:, :, 2].astype(np.uint16) >> 3
        rgb565 = (red | green | blue).astype(np.uint16)

        # LVGL is configured with LV_COLOR_16_SWAP=1, so swap RGB565 bytes before streaming.
        rgb565 = rgb565.byteswap()

        try:
            self.stream.write(rgb565.tobytes())
            self.stream.flush()
        except BrokenPipeError:
            raise SystemExit(0)


def run_preview(camera_index=0):
    capture = cv2.VideoCapture(camera_index)
    if not capture.isOpened():
        raise RuntimeError(f"Unable to open camera {camera_index}")

    window = LvWindow()

    try:
        while True:
            ok, frame = capture.read()
            if not ok:
                raise RuntimeError("Failed to read frame from camera")

            window.show(frame)
    finally:
        capture.release()


if __name__ == "__main__":
    run_preview()