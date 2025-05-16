import numpy as np


# Example of backpropagation in a multi-layer perceptron
class MLP:
    def __init__(self, layer_sizes, activations):
        self.layers = []
        self.activations = activations

        # Initialize weights and biases
        for i in range(len(layer_sizes) - 1):
            self.layers.append(
                {
                    "weights": np.random.uniform(
                        -1, 1, (layer_sizes[i], layer_sizes[i + 1])
                    ),
                    "biases": np.random.uniform(-0.01, 0.01, (layer_sizes[i + 1],)),
                }
            )

    def relu(self, x):
        return np.maximum(0, x)

    def relu_derivative(self, x):
        return np.where(x > 0, 1, 0)

    def identity(self, x):
        return x

    def identity_derivative(self, x):
        return np.ones_like(x)

    def get_activation_function(self, name):
        if name == "relu":
            return self.relu
        elif name == "identity":
            return self.identity
        else:
            raise ValueError(f"Activation {name} not supported")

    def get_activation_derivative(self, name):
        if name == "relu":
            return self.relu_derivative
        elif name == "identity":
            return self.identity_derivative
        else:
            raise ValueError(f"Activation {name} not supported")

    def forward(self, x):
        # Cache for storing intermediate values
        cache = {
            "pre_activations": [],
            "activations": [x],  # Input is the first activation
        }

        current = x
        for i, layer in enumerate(self.layers):
            # Linear transformation: z = x @ W + b
            z = np.dot(current, layer["weights"]) + layer["biases"]
            cache["pre_activations"].append(z)

            # Apply activation function
            activation_fn = self.get_activation_function(self.activations[i])
            current = activation_fn(z)
            cache["activations"].append(current)

        return current, cache

    def backward(self, x, y, cache, learning_rate=0.01):
        m = x.shape[0]  # Batch size

        # Compute the output error gradient (for MSE loss)
        # delta_L = (y_pred - y_true) for MSE with identity activation
        delta = cache["activations"][-1] - y

        # Initialize gradients
        gradients = []

        # Backpropagate through layers
        for i in reversed(range(len(self.layers))):
            # For the output layer, we already computed delta
            if i < len(self.layers) - 1:
                # Get activation derivative
                activation_derivative = self.get_activation_derivative(
                    self.activations[i]
                )

                # Compute error for current layer
                # delta_l = (delta_{l+1} @ W_{l+1}^T) * f'(z_l)
                delta = np.dot(
                    delta, self.layers[i + 1]["weights"].T
                ) * activation_derivative(cache["pre_activations"][i])

            # Compute gradients for current layer
            # dW_l = a_{l-1}^T @ delta_l
            dW = np.dot(cache["activations"][i].T, delta)
            # db_l = sum(delta_l)
            db = np.sum(delta, axis=0)

            # Store gradients (we prepend to maintain correct order)
            gradients.insert(0, {"dW": dW, "db": db})

        # Update weights and biases
        for i, grad in enumerate(gradients):
            self.layers[i]["weights"] -= learning_rate * grad["dW"]
            self.layers[i]["biases"] -= learning_rate * grad["db"]

        return gradients

    def __backward(self, x, y, cache, learning_rate=0.01):
        print("\n\nBACKWARD\n")
        # Compute the output error gradient (for MSE loss)
        # delta_L = (y_pred - y_true) for MSE with identity activation
        delta = cache["activations"][-1] - y
        print("backward delta 0:", delta.shape)

        # Backpropagate through layers
        for i in reversed(range(len(self.layers))):
            print(i)
            # Compute gradients for current layer
            # dW_l = a_{l-1}^T @ delta_l
            dW = np.dot(cache["activations"][i].T, delta)

            print("backward layer dw ", i, dW.shape)
            # db_l = sum(delta_l)
            db = np.sum(delta, axis=0)

            print("backward layer db ", i, db.shape)

            # Update weights and biases immediately
            self.layers[i]["weights"] -= learning_rate * dW
            self.layers[i]["biases"] -= learning_rate * db

            # If not the first layer, compute delta for the previous layer
            if i > 0:
                # Get activation derivative
                activation_derivative = self.get_activation_derivative(
                    self.activations[i - 1]
                )

                # Compute error for the previous layer
                # delta_l = (delta_{l+1} @ W_{l+1}^T) * f'(z_l)
                delta = np.dot(
                    delta, self.layers[i]["weights"].T
                ) * activation_derivative(cache["pre_activations"][i - 1])

    def train(self, x, y, epochs=100, learning_rate=0.01):
        for epoch in range(epochs):
            # Forward pass
            y_pred, cache = self.forward(x)

            # Compute loss (MSE)
            loss = np.mean((y_pred - y) ** 2)

            # Backward pass
            self.__backward(x, y, cache, learning_rate)

            if epoch % 10 == 0:
                print(f"Epoch {epoch}, Loss: {loss}")


# Example usage
layer_sizes = [2, 16, 16, 1]
activations = ["relu", "relu", "identity"]
mlp = MLP(layer_sizes, activations)

# Generate some sample data (XOR problem)
X = np.array([[0, 0], [0, 1], [1, 0], [1, 1]])
y = np.array([[0], [1], [1], [0]])

# Train the network
mlp.train(X, y, epochs=100, learning_rate=0.01)
