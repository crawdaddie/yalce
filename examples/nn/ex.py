import numpy as np


# Define a simple MLP class structure that matches your language's structure
class Layer:
    def __init__(self, weights, biases, activation):
        self.weights = weights  # numpy array
        self.biases = biases  # numpy array
        self.activation = activation  # function


class Network:
    def __init__(self, layers, loss_fn):
        self.layers = layers  # list of Layer objects
        self.loss = loss_fn  # function


# Activation functions
def relu(x):
    return np.maximum(0, x)


def identity(x):
    return x


# Activation derivatives
def relu_derivative(x):
    return np.where(x > 0, 1, 0)


def identity_derivative(x):
    return np.ones_like(x)


# Loss function
def mse_loss(predictions, targets):
    return np.mean((predictions - targets) ** 2)


# Forward pass similar to your current implementation
def forward(network, input_data):
    current = input_data
    print("Forward pass:")

    for i, layer in enumerate(network.layers):
        print(f"Processing layer {i}")
        # Matrix multiplication (weights * current)
        z = np.dot(layer.weights, current)
        # Add bias
        z_with_bias = z + layer.biases
        # Apply activation
        current = layer.activation(z_with_bias)

    return current


# Forward pass with caching for backpropagation
def forward_with_cache(network, input_data):
    # Initialize caches
    num_layers = len(network.layers)
    inputs = [None] * (num_layers + 1)
    pre_activations = [None] * num_layers
    activations = [None] * (num_layers + 1)

    # Input is first activation
    current = input_data
    inputs[0] = current
    activations[0] = current

    # Forward pass through each layer
    for i, layer in enumerate(network.layers):
        print(f"Processing layer {i}")

        # Store input to this layer
        inputs[i + 1] = current

        # Matrix multiplication
        z = np.dot(layer.weights, current)

        # Add bias
        z_with_bias = z + layer.biases

        # Store pre-activation
        pre_activations[i] = z_with_bias

        # Apply activation function
        current = layer.activation(z_with_bias)

        # Store activation
        activations[i + 1] = current

    # Return both output and cache for backpropagation
    cache = {
        "inputs": inputs,
        "pre_activations": pre_activations,
        "activations": activations,
    }

    return current, cache


# Backward pass (backpropagation)
def backward(network, cache, target, learning_rate):
    num_layers = len(network.layers)

    # Get output from cache
    final_output = cache["activations"][num_layers]

    # Initialize delta for output layer (derivative of loss with respect to output)
    delta = 2 * (final_output - target) / len(final_output)

    # Create new layers to store updated weights and biases
    new_layers = []

    # Go backwards through the layers
    for l in reversed(range(num_layers)):
        layer = network.layers[l]

        # Determine which activation derivative to use
        if layer.activation == relu:
            activation_derivative_fn = relu_derivative
        else:  # Assume identity
            activation_derivative_fn = identity_derivative

        # If not output layer, multiply delta by derivative of activation
        if l < num_layers - 1:
            z = cache["pre_activations"][l]
            dz = activation_derivative_fn(z)
            delta = delta * dz

        # Calculate gradients
        current_input = cache["inputs"][l]

        # Weight gradient: outer product of delta and input
        dW = np.outer(delta, current_input)

        # Bias gradient = delta
        db = delta

        # Update weights and biases
        new_weights = layer.weights - learning_rate * dW
        new_biases = layer.biases - learning_rate * db

        # Create new layer with updated weights and biases
        new_layer = Layer(new_weights, new_biases, layer.activation)

        # Insert at the beginning to maintain correct order
        new_layers.insert(0, new_layer)

        # Calculate delta for previous layer if not at input layer
        if l > 0:
            # Delta for previous layer = current layer weights transposed * current delta
            delta = np.dot(layer.weights.T, delta)

    # Create new network with updated layers
    new_network = Network(new_layers, network.loss)

    return new_network


# Training step (one example)
def train_step(network, input_data, target, learning_rate):
    # Forward pass with caching
    output, cache = forward_with_cache(network, input_data)

    # Backward pass to update weights
    updated_network = backward(network, cache, target, learning_rate)

    return updated_network


# Full training function
def train(network, inputs, targets, epochs, learning_rate):
    current_network = network

    for epoch in range(epochs):
        epoch_loss = 0

        for i in range(len(inputs)):
            input_data = inputs[i]
            target = targets[i]

            # Forward pass to calculate current loss
            output = forward(current_network, input_data)
            loss_value = mse_loss(output, target)
            epoch_loss += loss_value

            # Update network with backpropagation
            current_network = train_step(
                current_network, input_data, target, learning_rate
            )

        avg_loss = epoch_loss / len(inputs)
        print(f"Epoch {epoch+1}, Loss: {avg_loss:.6f}")

    return current_network


# Example usage
def main():
    # XOR problem inputs
    inputs = [np.array([0, 0]), np.array([0, 1]), np.array([1, 0]), np.array([1, 1])]

    # XOR problem outputs
    targets = [np.array([0]), np.array([1]), np.array([1]), np.array([0])]

    # Create network (2 inputs, hidden layer with 2 neurons, 1 output)
    hidden_layer = Layer(
        weights=np.random.randn(2, 2) * 0.1,
        biases=np.random.randn(2) * 0.1,
        activation=relu,
    )

    output_layer = Layer(
        weights=np.random.randn(1, 2) * 0.1,
        biases=np.random.randn(1) * 0.1,
        activation=identity,
    )

    network = Network([hidden_layer, output_layer], mse_loss)

    # Train the network
    trained_network = train(network, inputs, targets, epochs=1000, learning_rate=0.1)

    # Test the trained network
    for input_data, target in zip(inputs, targets):
        output = forward(trained_network, input_data)
        print(f"Input: {input_data}, Target: {target}, Prediction: {output}")


if __name__ == "__main__":
    main()
