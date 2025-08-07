#!/bin/bash

# Test script for YLC LSP server

echo "Building YLC LSP server..."

# Build the LSP server
make clean
make

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful!"

# Create a test YLC file
cat > test.ylc << 'EOF'
let x = 42;
let y = "hello";

let add = fn a b -> a + b;;

let result = add x 10
EOF

echo "Created test YLC file:"
cat test.ylc

echo ""
echo "LSP server built successfully at: ../../build/dev/lsp/ylc-lsp-server"
echo ""
echo "To test the server manually:"
echo "1. Start the server: ../../build/dev/lsp/ylc-lsp-server"
echo "2. Send LSP initialize request"
echo "3. Send textDocument/didOpen with test.ylc content"
echo ""
echo "You can also integrate this with VS Code or other editors that support LSP."
