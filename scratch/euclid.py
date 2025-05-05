def euclidean_sequencer(steps, pulses):
    pattern = [0] * steps  # Initialize an array of zeros

    if pulses > steps:
        raise ValueError("Pulses cannot be greater than steps")

    # Euclidean Algorithm
    quotient, remainder = divmod(steps, pulses)

    # Place pulses
    pos = 0
    for _ in range(pulses):
        pattern[pos] = 1
        pos = (pos + quotient) % steps

    # Handle remainders
    if remainder > 0:
        pos = 0
        for _ in range(remainder):
            pattern[pos] = 1
            pos = (pos + quotient + 1) % steps

    return pattern


# Example usage:
steps = 16
pulses = 7
euclidean_pattern = euclidean_sequencer(steps, pulses)
print(euclidean_pattern)
