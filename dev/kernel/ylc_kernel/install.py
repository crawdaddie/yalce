import os
import sys
import json
import argparse
import shutil
from pathlib import Path
from jupyter_client.kernelspec import KernelSpecManager
from tempfile import TemporaryDirectory

def install_ylc_kernel(ylc_path=None, user=True, prefix=None):
    """Install the YLC kernel in the Jupyter kernelspec directory."""
    if ylc_path is None:
        ylc_path = shutil.which("ylc")
        if ylc_path is None:
            raise ValueError("YLC executable not found in PATH. "
                            "Please specify the path using --ylc-path.")
    else:
        if not os.path.isfile(ylc_path):
            raise ValueError(f"YLC executable not found at {ylc_path}")
    
    kernel_json = {
        "argv": [sys.executable, "-m", "ylc_kernel", "-f", "{connection_file}"],
        "display_name": "YLC Language",
        "language": "ylc",
        "env": {"YLC_PATH": ylc_path}
    }
    
    with TemporaryDirectory() as td:
        os.chmod(td, 0o755)
        
        with open(os.path.join(td, 'kernel.json'), 'w') as f:
            json.dump(kernel_json, f, indent=2)
        
        logo_path = os.path.join(os.path.dirname(__file__), 'logo-64x64.png')
        if os.path.exists(logo_path):
            shutil.copy2(logo_path, os.path.join(td, 'logo-64x64.png'))
        
        print('Installing YLC kernel spec')
        KernelSpecManager().install_kernel_spec(td, 'ylc', user=user, prefix=prefix)
    
    print(f"YLC kernel installed. YLC executable path: {ylc_path}")
    return 0

def uninstall_ylc_kernel():
    """Uninstall the YLC kernel from Jupyter."""
    print('Uninstalling YLC kernel spec')
    try:
        KernelSpecManager().remove_kernel_spec('ylc')
        print("YLC kernel uninstalled")
        return 0
    except Exception as e:
        print(f"Error uninstalling YLC kernel: {e}")
        return 1

def main():
    parser = argparse.ArgumentParser(description='Install/uninstall YLC Jupyter kernel')
    subparsers = parser.add_subparsers(dest='command')
    
    install_parser = subparsers.add_parser('install', help='Install the YLC kernel')
    install_parser.add_argument('--ylc-path', type=str, help='Path to YLC executable')
    install_parser.add_argument('--user', action='store_true', help='Install for current user only')
    install_parser.add_argument('--sys-prefix', action='store_true', 
                             help='Install into sys.prefix (e.g. in virtual env)')
    
    subparsers.add_parser('uninstall', help='Uninstall the YLC kernel')
    
    args = parser.parse_args()
    
    if args.command == 'install':
        prefix = None
        if args.sys_prefix:
            prefix = sys.prefix
        return install_ylc_kernel(args.ylc_path, user=args.user, prefix=prefix)
    elif args.command == 'uninstall':
        return uninstall_ylc_kernel()
    else:
        parser.print_help()
        return 0

if __name__ == '__main__':
    sys.exit(main())
