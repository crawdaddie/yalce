import matplotlib.pyplot as plt
import numpy as np

# Step 1: Load the values from the file
y_values = np.loadtxt("sin_values.txt")

# Step 2: Generate the corresponding x-values
num_samples = len(y_values)
x_values = np.arange(len(y_values))

# Step 3: Plot the sine wave
plt.plot(x_values, y_values)
plt.title("Sine Wave Plot from values.txt")
plt.xlabel("X values")
plt.ylabel("Sine values")
plt.grid(True)  # Optionally, add a grid to the plot
plt.show()
