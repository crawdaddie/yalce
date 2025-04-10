from setuptools import setup
from setuptools.command.install import install
import os
import sys
import json
import shutil
from pathlib import Path

kernel_json = {
    "argv": [sys.executable, "-m", "ylc_kernel", "-f", "{connection_file}"],
    "display_name": "YLC Language",
    "language": "ylc",
}

class InstallWithKernelspec(install):
    def run(self):
        install.run(self)

        from jupyter_client.kernelspec import KernelSpecManager
        from tempfile import TemporaryDirectory
        with TemporaryDirectory() as td:
            os.chmod(td, 0o755)  
            with open(os.path.join(td, 'kernel.json'), 'w') as f:
                json.dump(kernel_json, f, indent=2)
            
            logo_path = os.path.join(os.path.dirname(__file__), 'ylc_kernel', 'logo-64x64.png')
            if os.path.exists(logo_path):
                shutil.copy2(logo_path, os.path.join(td, 'logo-64x64.png'))
            
            KernelSpecManager().install_kernel_spec(td, 'ylc', user=self.user, prefix=self.prefix)

setup(
    name="ylc_kernel",
    version="0.1.0",
    packages=["ylc_kernel"],
    description="YLC Language Jupyter Kernel",
    author="Your Name",
    author_email="adam.a.juraszek.@googlemail.com",
    url="https://github.com/crawdaddie/ylc_kernel",
    install_requires=[
        "jupyter_client",
        "ipykernel",
        "ipython"
    ],
    cmdclass={
        'install': InstallWithKernelspec,
    },
    classifiers=[
        "Framework :: Jupyter",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
    ],
)
