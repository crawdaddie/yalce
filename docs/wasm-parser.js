import { WASI, OpenFile, ConsoleStdout, File } from "https://cdn.jsdelivr.net/npm/@bjorn3/browser_wasi_shim@0.3.0/dist/index.js";

let wasmInstance = null;
let wasmMemory = null;
let context = null;
let wasi = null;

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

// Clear all
function clearAll() {
  document.getElementById('input').value = '';
  document.getElementById('output').textContent = 'Waiting for input...';
}

// Expose functions to window for onclick handlers
window.parseCode = parseCode;
window.clearAll = clearAll;

// Load WASM on page load
window.addEventListener('load', loadWasm);
