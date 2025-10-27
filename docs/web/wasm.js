import { WASI, OpenFile, ConsoleStdout, File } from "https://cdn.jsdelivr.net/npm/@bjorn3/browser_wasi_shim@0.3.0/dist/index.js";

let wasmInstance = null;
let wasmMemory = null;
let context = null;
let wasi = null;
let repl = null;

// REPL for incremental WASM execution
class YLCWasmREPL {
  constructor(wasmInstance, wasmMemory, context) {
    this.wasmInstance = wasmInstance;
    this.wasmMemory = wasmMemory;
    this.context = context;
    this.codeHistory = [];
    this.lastModule = null;
  }

  addCode(input) {
    this.codeHistory.push(input);
  }

  getFullProgram() {
    return this.codeHistory.join('\n');
  }

  clear() {
    this.codeHistory = [];
    this.lastModule = null;
  }

  async execute() {
    const fullProgram = this.getFullProgram();
    if (!fullProgram.trim()) {
      return null;
    }
    console.log(fullProgram);

    try {
      // Parse and typecheck entire program
      const inputPtr = this.writeString(fullProgram);
      const astPtr = this.wasmInstance.exports.parse_and_analyze(
        this.context, inputPtr, 0
      );

      if (!astPtr || astPtr === 0) {
        this.wasmInstance.exports.free(inputPtr);
        throw new Error("Parse or type error");
      }

      // Generate executable WASM module
      const moduleSizePtr = this.wasmInstance.exports.malloc(4);
      const modulePtr = this.wasmInstance.exports.generate_executable_module(
        astPtr, moduleSizePtr
      );

      if (!modulePtr || modulePtr === 0) {
        this.wasmInstance.exports.free(inputPtr);
        this.wasmInstance.exports.free(moduleSizePtr);
        throw new Error("Failed to generate WASM module");
      }

      // Read module size
      const dataView = new DataView(this.wasmMemory.buffer);
      const moduleSize = dataView.getUint32(moduleSizePtr, true);

      // Copy module bytes
      const moduleBytes = new Uint8Array(this.wasmMemory.buffer, modulePtr, moduleSize);
      const moduleCopy = new Uint8Array(moduleBytes);

      // Free C memory
      this.wasmInstance.exports.free(inputPtr);
      this.wasmInstance.exports.free(modulePtr);
      this.wasmInstance.exports.free(moduleSizePtr);

      // Instantiate the generated module
      const imports = this.createImports();
      const { instance } = await WebAssembly.instantiate(moduleCopy, imports);

      this.lastModule = instance;

      // Execute the main function
      if (instance.exports.repl_eval) {
        const result = instance.exports.repl_eval();
        return result;
      }

      return null;
    } catch (error) {
      console.error('REPL execution error:', error);
      throw error;
    }
  }

  writeString(str) {
    const encoder = new TextEncoder();
    const bytes = encoder.encode(str + '\0');
    const ptr = this.wasmInstance.exports.malloc(bytes.length);
    const mem = new Uint8Array(this.wasmMemory.buffer, ptr, bytes.length);
    mem.set(bytes);
    return ptr;
  }

  createImports() {
    return {
      env: {
        print_int: (x) => {
          console.log('[output]', x);
          const outputEl = document.getElementById('output');
          if (outputEl) {
            outputEl.textContent += `Result: ${x}\n`;
          }
        },
        print_float: (x) => {
          console.log('[output]', x);
          const outputEl = document.getElementById('output');
          if (outputEl) {
            outputEl.textContent += `Result: ${x}\n`;
          }
        },
      }
    };
  }
}

// Status management
function setStatus(message, type) {
  const statusEl = document.getElementById('status');
  statusEl.textContent = message;
  statusEl.className = `status ${type}`;
  statusEl.style.display = 'block';
  if (type !== 'loading') {
    setTimeout(() => {
      statusEl.style.display = 'none';
    }, 3000);
  }
}

// Load WASM module
async function loadWasm() {
  setStatus('Loading WASM module...', 'loading');
  try {
    const response = await fetch('jit.wasm');
    const buffer = await response.arrayBuffer();

    // Create WASI instance with proper file descriptors
    const decoder = new TextDecoder("utf-8");
    const fds = [
      new OpenFile(new File([])), // stdin
      new ConsoleStdout((buffer) => {
        const text = decoder.decode(buffer, { stream: true });
        console.log("[stdout]", text);
        const outputEl = document.getElementById('output');
        if (outputEl) {
          outputEl.textContent += text;
        }
      }),
      new ConsoleStdout((buffer) => {
        const text = decoder.decode(buffer, { stream: true });
        console.error("[stderr]", text);
        const outputEl = document.getElementById('output');
        if (outputEl) {
          outputEl.textContent += '[ERROR] ' + text;
        }
      }),
    ];
    wasi = new WASI([], [], fds);

    // Custom env imports for functions not provided by WASI
    const envImports = {
      // YLC-specific functions (not available in browser)
      repl_input: () => 0,
      read_script: () => 0,
      get_dirname: () => 0,
      resolve_relative_path: () => 0,
      normalize_path: () => 0,
      init_readline: () => { },
      add_completion_item: () => { },
      get_mod_name_from_path_identifier: () => 0,
    };

    const imports = {
      wasi_snapshot_preview1: wasi.wasiImport,
      env: envImports
    };

    const result = await WebAssembly.instantiate(buffer, imports);
    wasmInstance = result.instance;
    wasmMemory = wasmInstance.exports.memory;

    // Initialize WASI
    wasi.inst = wasmInstance;

    // Create context
    if (wasmInstance.exports.create_simple_context) {
      context = wasmInstance.exports.create_simple_context();
      console.log('Created context:', context);
    }

    // Initialize REPL
    repl = new YLCWasmREPL(wasmInstance, wasmMemory, context);
    console.log('REPL initialized');

    setStatus('WASM module loaded successfully!', 'success');
    console.log('WASM exports:', Object.keys(wasmInstance.exports));
  } catch (error) {
    setStatus(`Failed to load WASM: ${error.message}`, 'error');
    console.error('WASM load error:', error);
  }
}

// Helper to write string to WASM memory
function writeStringToMemory(str) {
  const encoder = new TextEncoder();
  const bytes = encoder.encode(str + '\0'); // null-terminated
  const ptr = wasmInstance.exports.malloc(bytes.length);
  const mem = new Uint8Array(wasmMemory.buffer, ptr, bytes.length);
  mem.set(bytes);
  return ptr;
}

// Parse code
function parseCode() {
  const input = document.getElementById('input').value;
  const outputEl = document.getElementById('output');

  if (!wasmInstance) {
    outputEl.textContent = 'Error: WASM not loaded yet';
    setStatus('WASM module not ready', 'error');
    return;
  }

  if (!input.trim()) {
    outputEl.textContent = 'Please enter some code';
    return;
  }

  try {
    // Add separator for new parse attempt (keep history)
    if (outputEl.textContent && outputEl.textContent !== 'Waiting for input...') {
    } else {
      outputEl.textContent = '';
    }

    setStatus('Parsing...', 'loading');

    // Try to call the parse function
    const inputPtr = writeStringToMemory(input);
    let astPtr = null;

    if (wasmInstance.exports.wasm_parse_and_analyze) {
      astPtr = wasmInstance.exports.wasm_parse_and_analyze(context, inputPtr);
    } else if (wasmInstance.exports.parse_and_analyze) {
      astPtr = wasmInstance.exports.parse_and_analyze(context, inputPtr, 0);
    }

    // Free the input string
    if (wasmInstance.exports.free) {
      wasmInstance.exports.free(inputPtr);
    }

    if (astPtr && astPtr !== 0) {
      setStatus('Parse successful!', 'success');
      // Output is shown via stdout callback
    } else {
      outputEl.textContent += '\n❌ Parse failed';
      setStatus('Parse failed - see console', 'error');
    }
  } catch (error) {
    outputEl.textContent = `❌ Error: ${error.message}\n\n${error.stack}`;
    setStatus(`Error: ${error.message}`, 'error');
    console.error('Parse error:', error);
  }
}

// Initialize terminal with prompt
function initTerminal() {
  const terminal = document.getElementById('repl-terminal');
  if (terminal) {
    terminal.value = 'λ > ';
    terminal.focus();
    // Move cursor to end
    terminal.setSelectionRange(terminal.value.length, terminal.value.length);
  }
}

// Get current input (text after last prompt)
function getCurrentInput() {
  const terminal = document.getElementById('repl-terminal');
  const content = terminal.value;
  const lastPromptIndex = content.lastIndexOf('λ > ');

  if (lastPromptIndex === -1) {
    return '';
  }

  return content.substring(lastPromptIndex + 4); // 4 = length of 'λ > '
}

// Execute code in REPL
async function executeCurrentLine() {
  if (!repl) {
    setStatus('REPL not ready', 'error');
    return;
  }

  let input = getCurrentInput().trim();

  if (!input) {
    return;
  }

  // Add semicolon if needed
  if (input[input.length - 1] != ";") {
    input = input + ";";
  }

  const terminal = document.getElementById('repl-terminal');

  try {
    setStatus('Executing...', 'loading');

    // Add code to REPL history
    repl.addCode(input);

    // Execute
    const result = await repl.execute();

    // Append result to terminal
    if (result !== null && result !== undefined) {
      terminal.value += `\n=> ${result}\n`;
      setStatus('Execution successful!', 'success');
    } else {
      terminal.value += `\n`;
      setStatus('Executed', 'success');
    }

    // Add new prompt
    terminal.value += 'λ > ';

    // Scroll to bottom and focus
    terminal.scrollTop = terminal.scrollHeight;
    terminal.setSelectionRange(terminal.value.length, terminal.value.length);

  } catch (error) {
    terminal.value += `\n❌ Error: ${error.message}\n`;
    setStatus(`Error: ${error.message}`, 'error');
    console.error('Execution error:', error);

    // Remove last code from history on error
    repl.codeHistory.pop();

    // Add new prompt
    terminal.value += 'λ > ';
    terminal.scrollTop = terminal.scrollHeight;
    terminal.setSelectionRange(terminal.value.length, terminal.value.length);
  }
}

// Handle keydown in terminal
function handleTerminalKeydown(e) {
  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault();
    executeCurrentLine();
  }
}

// Clear REPL
function clearREPL() {
  const terminal = document.getElementById('repl-terminal');
  terminal.value = 'λ > ';
  terminal.focus();
  terminal.setSelectionRange(terminal.value.length, terminal.value.length);

  if (repl) {
    repl.clear();
  }

  setStatus('', '');
  document.getElementById('status').style.display = 'none';
}

// Set up terminal event listeners
function setupTerminal() {
  const terminal = document.getElementById('repl-terminal');
  if (terminal) {
    terminal.addEventListener('keydown', handleTerminalKeydown);
    initTerminal();
  }
}

// Expose functions to window for onclick handlers
window.parseCode = parseCode;
window.clearREPL = clearREPL;

// Load WASM on page load
window.addEventListener('load', () => {
  loadWasm();
  setupTerminal();
});
