# YLC Jupyter Kernel

A Jupyter kernel for the YLC language, allowing you to use YLC in Jupyter notebooks and other Jupyter interfaces.

## Requirements

- Python 3.7+
- Jupyter
- YLC interpreter installed and accessible in your PATH

## Installation

You can install the kernel using pip:

```bash
# Install with pip
pip install .

# Install the kernel
python -m ylc_kernel.install install
```

Or if YLC is not in your PATH, specify its location:

```bash
python -m ylc_kernel.install install --ylc-path /path/to/ylc
```

### Development Installation

For development, you can install in editable mode:

```bash
pip install -e .
python -m ylc_kernel.install install
```

## Uninstalling

To remove the kernel:

```bash
python -m ylc_kernel.install uninstall
pip uninstall ylc-kernel
```

## Using the Kernel

After installation, the YLC kernel will be available in Jupyter:

1. Start Jupyter Notebook or Jupyter Lab:
   ```bash
   jupyter notebook
   ```
   or
   ```bash
   jupyter lab
   ```

2. Create a new notebook and select "YLC Language" as the kernel.

3. Write YLC code in cells and execute them.

## How it Works

The kernel starts a YLC interpreter in interactive mode (`ylc -i`) and communicates with it via stdin/stdout. Code entered in notebook cells is sent to the interpreter, and the output is captured and displayed in the notebook.

## Troubleshooting

If the kernel fails to start, check that:

1. The YLC interpreter is installed and in your PATH
2. You have sufficient permissions to create kernel specs
3. The Jupyter installation is working correctly

## License

MIT
