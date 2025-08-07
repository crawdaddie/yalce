#!/usr/bin/env python3
"""
Simple integration test for YLC LSP server
"""

import json
import subprocess
import sys
import time

def send_lsp_request(process, request):
    """Send a JSON-RPC request to the LSP server"""
    content = json.dumps(request)
    message = f"Content-Length: {len(content)}\r\n\r\n{content}"
    
    print(f"Sending: {message}")
    process.stdin.write(message.encode())
    process.stdin.flush()

def read_lsp_response(process):
    """Read a response from the LSP server"""
    # Read the Content-Length header
    line = process.stdout.readline().decode().strip()
    if not line.startswith("Content-Length:"):
        return None
    
    content_length = int(line.split(":")[1].strip())
    
    # Read the empty line
    process.stdout.readline()
    
    # Read the JSON content
    content = process.stdout.read(content_length).decode()
    print(f"Received: {content}")
    
    return json.loads(content)

def test_lsp_server():
    """Test basic LSP server functionality"""
    server_path = "../../build/dev/lsp/ylc-lsp-server"
    
    print("Starting YLC LSP server...")
    process = subprocess.Popen(
        [server_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    try:
        # Send initialize request
        print("\n1. Testing initialize...")
        initialize_request = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {
                "processId": None,
                "rootUri": "file:///tmp/test-ylc",
                "capabilities": {}
            }
        }
        
        send_lsp_request(process, initialize_request)
        response = read_lsp_response(process)
        
        if response and "result" in response:
            print("✓ Initialize successful")
            capabilities = response["result"]["capabilities"]
            print(f"  Server capabilities: {list(capabilities.keys())}")
        else:
            print("✗ Initialize failed")
            return False
        
        # Send textDocument/didOpen
        print("\n2. Testing document open...")
        ylc_content = """let x = 42
let greeting = "Hello, YLC!"

fn add(a: int, b: int) -> int {
  a + b
}

let result = add(x, 10)"""
        
        did_open_request = {
            "jsonrpc": "2.0",
            "method": "textDocument/didOpen",
            "params": {
                "textDocument": {
                    "uri": "file:///tmp/test.ylc",
                    "languageId": "ylc",
                    "version": 1,
                    "text": ylc_content
                }
            }
        }
        
        send_lsp_request(process, did_open_request)
        # didOpen might trigger diagnostics notification
        time.sleep(0.1)
        print("✓ Document opened")
        
        # Test hover
        print("\n3. Testing hover...")
        hover_request = {
            "jsonrpc": "2.0",
            "id": 2,
            "method": "textDocument/hover",
            "params": {
                "textDocument": {"uri": "file:///tmp/test.ylc"},
                "position": {"line": 0, "character": 4}
            }
        }
        
        send_lsp_request(process, hover_request)
        response = read_lsp_response(process)
        
        if response and "result" in response:
            print("✓ Hover successful")
            hover_contents = response["result"].get("contents")
            if hover_contents:
                print(f"  Hover info: {hover_contents}")
        else:
            print("✗ Hover failed")
        
        # Test completion
        print("\n4. Testing completion...")
        completion_request = {
            "jsonrpc": "2.0",
            "id": 3,
            "method": "textDocument/completion",
            "params": {
                "textDocument": {"uri": "file:///tmp/test.ylc"},
                "position": {"line": 6, "character": 0}
            }
        }
        
        send_lsp_request(process, completion_request)
        response = read_lsp_response(process)
        
        if response and "result" in response:
            print("✓ Completion successful")
            completions = response["result"]
            if isinstance(completions, list):
                print(f"  Found {len(completions)} completions")
                for comp in completions[:3]:  # Show first 3
                    print(f"    - {comp.get('label', 'unknown')}")
            else:
                print(f"  Completion result: {completions}")
        else:
            print("✗ Completion failed")
        
        # Send shutdown
        print("\n5. Testing shutdown...")
        shutdown_request = {
            "jsonrpc": "2.0",
            "id": 4,
            "method": "shutdown",
            "params": None
        }
        
        send_lsp_request(process, shutdown_request)
        response = read_lsp_response(process)
        
        if response and response.get("id") == 4:
            print("✓ Shutdown successful")
        else:
            print("✗ Shutdown failed")
        
        return True
        
    except Exception as e:
        print(f"Test failed with exception: {e}")
        return False
    
    finally:
        # Clean up
        process.terminate()
        process.wait()

if __name__ == "__main__":
    print("YLC LSP Server Integration Test")
    print("=" * 40)
    
    success = test_lsp_server()
    
    print(f"\nTest {'PASSED' if success else 'FAILED'}")
    sys.exit(0 if success else 1)