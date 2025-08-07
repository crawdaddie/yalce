# YLC Language Server Protocol (LSP) Implementation

A comprehensive Language Server Protocol implementation for the YLC programming language, providing rich IDE features including syntax analysis, type information, completions, and diagnostics.

## Features

### Core LSP Capabilities
- ✅ **Document Management**: Full document synchronization with editors
- ✅ **Diagnostics**: Real-time syntax and type error reporting
- ✅ **Hover Information**: Type and symbol information on hover
- ✅ **Code Completion**: Intelligent suggestions for keywords, types, and symbols
- ✅ **Document Formatting**: Code formatting support (infrastructure ready)

### YLC Language Integration
- **Parser Integration**: Uses YLC's flex/bison parser for syntax analysis
- **Type System**: Integrates with Hindley-Milner type inference system
- **Symbol Tables**: Access to YLC's symbol resolution and scope analysis
- **Error Reporting**: Native YLC error messages converted to LSP diagnostics

## Architecture

```
┌─────────────────┐    JSON-RPC    ┌─────────────────┐
│   LSP Client    │ ←============→ │  LSP Server     │
│  (VS Code, etc) │                │                 │
└─────────────────┘                └─────────────────┘
                                           │
                                           ▼
                                   ┌─────────────────┐
                                   │ YLC Integration │
                                   │                 │
                                   │ • Parser        │
                                   │ • Type Checker  │
                                   │ • Symbol Table  │
                                   └─────────────────┘
```

### Key Components

1. **`lsp_server.c/h`**: Core LSP protocol handling and message routing
2. **`protocol.c/h`**: JSON-RPC message parsing and response creation
3. **`ylc_integration.c/h`**: Bridge between LSP and YLC language components
4. **`main.c`**: Server entry point and lifecycle management

## Building

### Prerequisites
```bash
# Install json-c library
brew install json-c

# Ensure YLC is built
make -C ../.. 
```

### Build Commands
```bash
# Build the LSP server
make

# Clean build artifacts  
make clean

# Run tests
./test_lsp.sh
python3 test_integration.py
```

## Usage

### Manual Testing
```bash
# Start the server
./build/dev/lsp/ylc-lsp-server

# The server accepts JSON-RPC messages on stdin
# Example initialize request:
Content-Length: 137

{"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {"processId": null, "rootUri": "file:///path/to/project", "capabilities": {}}}
```

### Editor Integration

#### VS Code
1. Install the YLC extension (configuration provided in `ylc-lsp-config.json`)
2. Configure the server path in settings:
   ```json
   {
     "ylc.lsp.serverPath": "./build/dev/lsp/ylc-lsp-server"
   }
   ```

#### Vim/Neovim
```lua
-- Using nvim-lspconfig
local lspconfig = require('lspconfig')
local configs = require('lspconfig.configs')

configs.ylc = {
  default_config = {
    cmd = {'./build/dev/lsp/ylc-lsp-server'},
    filetypes = {'ylc'},
    root_dir = lspconfig.util.root_pattern('.git', 'Makefile'),
    settings = {}
  }
}

lspconfig.ylc.setup{}
```

#### Emacs
```elisp
;; Using lsp-mode
(add-to-list 'lsp-language-id-configuration '(ylc-mode . "ylc"))
(lsp-register-client
 (make-lsp-client :new-connection (lsp-stdio-connection "./build/dev/lsp/ylc-lsp-server")
                  :major-modes '(ylc-mode)
                  :server-id 'ylc-ls))
```

## Supported LSP Methods

### Lifecycle
- `initialize` - Server initialization and capability negotiation
- `shutdown` - Graceful server shutdown
- `exit` - Process termination

### Text Document Synchronization  
- `textDocument/didOpen` - Document opened in editor
- `textDocument/didChange` - Document content changed
- `textDocument/didClose` - Document closed in editor

### Language Features
- `textDocument/hover` - Show type/symbol information
- `textDocument/completion` - Code completion suggestions
- `textDocument/formatting` - Format document (planned)
- `textDocument/publishDiagnostics` - Send syntax/type errors

## YLC Language Features Supported

### Syntax Elements
- **Keywords**: `let`, `fn`, `match`, `if`, `else`, `import`, `extern`, `type`, etc.
- **Types**: `int`, `double`, `string`, `bool`, `array`, `list`
- **Literals**: Numbers, strings, booleans
- **Functions**: Lambda expressions, function applications
- **Pattern Matching**: Match expressions with guards
- **Data Types**: ADTs, records, tuples
- **Modules**: Import/export system

### Advanced Features
- **Coroutines**: `yield` expressions and coroutine scheduling
- **Type Inference**: Hindley-Milner type system integration
- **Closures**: Lexical scoping and closure conversion
- **FFI**: External function declarations

## Development

### Project Structure
```
dev/lsp/
├── src/
│   ├── lsp_server.c/h      # Core server implementation
│   ├── protocol.c/h        # JSON-RPC protocol handling
│   ├── ylc_integration.c/h # YLC language integration
│   └── main.c              # Server entry point
├── Makefile                # Build configuration
├── test_lsp.sh            # Build and basic test script
├── test_integration.py    # Comprehensive integration tests
└── README.md              # This file
```

### Adding New Features

1. **New LSP Method**: Add handler in `lsp_server.c` main loop
2. **YLC Integration**: Extend functions in `ylc_integration.c`
3. **Protocol Extensions**: Update `protocol.c` if needed

### Testing

```bash
# Build test
make clean && make

# Integration test  
python3 test_integration.py

# Manual testing
echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}' | ./build/dev/lsp/ylc-lsp-server
```

## Future Enhancements

### Planned Features
- [ ] **Go to Definition**: Navigate to symbol definitions
- [ ] **Find References**: Find all uses of a symbol  
- [ ] **Rename Symbol**: Rename symbols across files
- [ ] **Document Symbols**: Outline view of file structure
- [ ] **Workspace Symbols**: Project-wide symbol search
- [ ] **Code Actions**: Quick fixes and refactorings
- [ ] **Semantic Highlighting**: Advanced syntax highlighting

### Parser Integration Improvements
- [ ] **Real-time Parsing**: Direct AST integration instead of subprocess calls
- [ ] **Incremental Updates**: Parse only changed portions
- [ ] **Error Recovery**: Better handling of partial/invalid syntax
- [ ] **Position Mapping**: Precise source location tracking

### Type System Integration
- [ ] **Hover Details**: Rich type information with inferred types
- [ ] **Type Errors**: Detailed type mismatch diagnostics  
- [ ] **Completion Context**: Type-aware completions
- [ ] **Signature Help**: Function parameter hints

## Contributing

1. Follow the existing code style and patterns
2. Add tests for new features
3. Update documentation
4. Ensure compatibility with YLC language changes

## License

This LSP implementation is part of the YLC project and follows the same licensing terms.
