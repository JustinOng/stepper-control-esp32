import sys
import serial
import tkinter as tk
# https://stackoverflow.com/a/3968517

if len(sys.argv) < 2:
    sys.exit(f"Usage: {0} <serial port eg COM10>")

ser = serial.Serial(sys.argv[1], 115200)


class App:
    def __init__(self):
        self.root = tk.Tk()
        self._job = None
        self.slider = tk.Scale(self.root, from_=0, to=300,
                               length=500, sliderlength=50,
                               tickinterval=50,
                               orient="horizontal",
                               command=self.updateValue)
        self.slider.pack()
        tk.Button(self.root, text='Home', command=self.home).pack()
        self.root.mainloop()

    def updateValue(self, event):
        if self._job:
            self.root.after_cancel(self._job)
        self._job = self.root.after(100, self._writePosition)

    def _writePosition(self):
        self._job = None
        print(f"new value: {self.slider.get()}")
        ser.write(f"move 0 {self.slider.get()}\n".encode("utf8"))

    def home(self):
        ser.write(b"home 0\n")


app = App()
