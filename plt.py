import matplotlib.pyplot as plt
import numpy as np

# Step 1: Load the values from the file
sinc, window, phase = np.loadtxt("blit.txt", delimiter=" ", unpack=True)
# Step 2: Plot the arrays
plt.plot(sinc, label="Sinc", color="blue")
plt.plot(window, label="Window", color="green")
phase = phase / np.pi
plt.plot(phase, label="Phase", color="red")

# Step 3: Add labels and legend
plt.xlabel("Index")
plt.ylabel("Value")
plt.title("Arrays Plot")
plt.legend()

# Step 4: Show plot
plt.show()
