import { WASI, OpenFile, ConsoleStdout, File } from "https://cdn.jsdelivr.net/npm/@bjorn3/browser_wasi_shim@0.3.0/dist/index.js";

// ylc-wasm contract (see wasm/main.c):
//   ylc_wasm_init()                 -> i32  (status; 0 = ok)
//   ylc_wasm_compile(srcPtr, lenPtr) -> ptr  (heap ptr to a wasm module;
//                                             its byte length is written to
//                                             *lenPtr as a little-endian u32;
//                                             0 on failure)
//   ylc_wasm_dump_mir(srcPtr)       -> i32  (status; prints MIR to wasi stdout)
//
// The compiled module is the *output* of running the YLC frontend (lex,
// parse, typecheck, escape analysis, MIR build + passes) and then lowering
// the MirProgram straight to wasm — no LLVM involvement. js instantiates
// that generated module and calls its exported entry point to evaluate.
let wasmInstance = null;
let wasmMemory = null;
let wasi = null;
let repl = null;

// Generated YLC modules import this runtime surface from their host. The
// names below mirror the env the legacy backend_wasm emitted; wasm/main.c
// will reference the same imports once the MIR->wasm lowering is filled
// in. Keep them here so the runtime contract lives in one place.
const GENERATED_MODULE_IMPORTS = (instance, memory) => ({
  env: {
    memory,
    // Integer arithmetic + I/O builtins the lowering will emit calls to.
    // All are no-op stubs for now; replace with real implementations as
    // the lowering in wasm/main.c grows.
    ylc_print_int: (v) => console.log("[ylc] int:", v),
    ylc_print_string: (ptr) => {
      const view = new DataView(memory.buffer);
      let len = 0; let p = ptr;
      while (view.getUint8(p) !== 0) { len++; p++; }
      const bytes = new Uint8Array(memory.buffer, ptr, len);
      console.log("[ylc] str:", new TextDecoder().decode(bytes));
    },
    ylc_abort: () => { throw new Error("ylc runtime abort"); },
  },
});

// REPL for incremental WASM execution.
class YLCWasmREPL {
  constructor(wasmInstance, wasmMemory) {
    this.wasmInstance = wasmInstance;
    this.wasmMemory = wasmMemory;
  }

  clear() {}

  // Compile the most recent REPL line into a wasm module and run it.
  // Returns whatever the generated module's entry function returns, or
  // null if there is no entry point.
  async execute(source) {
    if (!source || !source.trim()) {
      return null;
    }
    const exports = this.wasmInstance.exports;

    // Hand the source string to the compiler.
    const srcPtr = this.writeString(source);
    const lenPtr = exports.malloc(4);
    new DataView(this.wasmMemory.buffer).setUint32(lenPtr, 0, true);

    const modulePtr = exports.ylc_wasm_compile(srcPtr, lenPtr);
    exports.free(srcPtr);

    if (!modulePtr || modulePtr === 0) {
      exports.free(lenPtr);
      throw new Error("YLC compile failed (see console for frontend errors)");
    }

    // Copy the generated module bytes out of linear memory, then free.
    const dataView = new DataView(this.wasmMemory.buffer);
    const moduleSize = dataView.getUint32(lenPtr, true);
    const moduleBytes = new Uint8Array(
      this.wasmMemory.buffer, modulePtr, moduleSize
    ).slice();
    exports.free(modulePtr);
    exports.free(lenPtr);

    // Instantiate the generated module against the runtime imports.
    const imports =
      GENERATED_MODULE_IMPORTS(this.wasmInstance, this.wasmMemory);
    const { instance } = await WebAssembly.instantiate(moduleBytes, imports);

    // Call the generated module's entry point. The lowering in
    // wasm/main.c is expected to export `ylc_entry` (or `eval` for back
    // compat with the legacy backend); call whichever exists.
    const entry = instance.exports.ylc_entry || instance.exports.eval;
    return entry ? entry() : null;
  }

  writeString(str) {
    const encoder = new TextEncoder();
    const bytes = encoder.encode(str + '\0');
    const ptr = this.wasmInstance.exports.malloc(bytes.length);
    const mem = new Uint8Array(this.wasmMemory.buffer, ptr, bytes.length);
    mem.set(bytes);
    return ptr;
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
        const terminal = document.getElementById('repl-terminal');
        if (terminal) {
          terminal.appendChild(document.createTextNode(text));
          terminal.scrollTop = terminal.scrollHeight;
        }
      }),
      new ConsoleStdout((buffer) => {
        const text = decoder.decode(buffer, { stream: true });
        console.error("[stderr]", text);
        const terminal = document.getElementById('repl-terminal');
        if (terminal) {
          terminal.appendChild(document.createTextNode('[ERROR] ' + text));
          terminal.scrollTop = terminal.scrollHeight;
        }
      }),
    ];
    wasi = new WASI([], [], fds);

    // Custom env imports for functions not provided by WASI.
    // parse.c references these (get_dirname/normalize_path/...) for file
    // imports, which the browser host never exercises; stub them to no-ops
    // so wasm-ld's --allow-undefined link resolves at instantiation time.
    const envImports = {
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

    // Initialize WASI (so printf/fprintf from the frontend route to the
    // console via the fds set up above).
    wasi.inst = wasmInstance;

    // One-time compiler init: module registry + builtin types + config.
    if (wasmInstance.exports.ylc_wasm_init) {
      const status = wasmInstance.exports.ylc_wasm_init();
      if (status !== 0) {
        throw new Error(`ylc_wasm_init failed (status ${status})`);
      }
    } else {
      throw new Error("jit.wasm missing ylc_wasm_init export; rebuild with `make -C wasm`");
    }

    repl = new YLCWasmREPL(wasmInstance, wasmMemory);
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
const PROMPT = 'λ ';

// Create a prompt span element
function createPrompt() {
  const span = document.createElement('span');
  span.className = 'prompt';
  span.textContent = PROMPT;
  span.contentEditable = 'false';
  return span;
}

// Move cursor to end of contenteditable
function moveCursorToEnd(element) {
  element.focus();
  const range = document.createRange();
  const sel = window.getSelection();
  range.selectNodeContents(element);
  range.collapse(false);
  sel.removeAllRanges();
  sel.addRange(range);
}

// Initialize terminal with prompt
function initTerminal() {
  const terminal = document.getElementById('repl-terminal');
  if (terminal) {
    terminal.innerHTML = '';
    terminal.appendChild(createPrompt());

    const paramsString = window.location.search;
    const searchParams = new URLSearchParams(paramsString);
    const codeInput = searchParams.get("code"); // a
    if (codeInput) {
      terminal.appendChild(document.createTextNode(codeInput));
    }
    moveCursorToEnd(terminal);
  }
}

// Get current input (text after last prompt)
function getCurrentInput() {
  const terminal = document.getElementById('repl-terminal');
  const text = terminal.textContent;
  const lastPromptIndex = text.lastIndexOf(PROMPT);

  if (lastPromptIndex === -1) {
    return '';
  }

  return text.substring(lastPromptIndex + PROMPT.length);
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

    // Compile + execute the current line.
    console.log(`execute '${input}'`)

    // Execute (compile -> instantiate generated module -> run entry)
    const result = await repl.execute(input);

    // Append result to terminal
    if (result !== null && result !== undefined) {
      terminal.appendChild(document.createTextNode(`\n> ${result}\n`));
    } else {
      terminal.appendChild(document.createTextNode(`\n`));
    }

    // Add new prompt
    terminal.appendChild(createPrompt());

    // Scroll to bottom and focus
    terminal.scrollTop = terminal.scrollHeight;
    moveCursorToEnd(terminal);

  } catch (error) {
    terminal.appendChild(document.createTextNode(`\n❌ Error: ${error.message}\n`));
    // setStatus(`Error: ${error.message}`, 'error');
    console.error('Execution error:', error);

    // Add new prompt
    terminal.appendChild(createPrompt());
    terminal.scrollTop = terminal.scrollHeight;
    moveCursorToEnd(terminal);
  }
}

// Get the last prompt element
function getLastPrompt() {
  const terminal = document.getElementById('repl-terminal');
  const prompts = terminal.querySelectorAll('.prompt');
  return prompts[prompts.length - 1];
}

// Check if cursor/selection is in the editable area (after last prompt)
function isInEditableArea() {
  const sel = window.getSelection();
  if (!sel.rangeCount) return false;

  const lastPrompt = getLastPrompt();
  if (!lastPrompt) return true;

  const range = sel.getRangeAt(0);
  const terminal = document.getElementById('repl-terminal');

  // Get position of last prompt in terminal
  let node = lastPrompt;
  let foundPrompt = false;

  // Check if the cursor is after the last prompt
  while (node) {
    if (node === range.startContainer || node.contains(range.startContainer)) {
      // Found where cursor is - check if it's after the prompt
      return foundPrompt;
    }
    if (node === lastPrompt) {
      foundPrompt = true;
    }
    node = node.nextSibling;
  }

  return foundPrompt;
}

// Prevent cursor from moving before last prompt
function enforceEditableArea(e) {
  const lastPrompt = getLastPrompt();
  if (!lastPrompt) return;

  const sel = window.getSelection();
  if (!sel.rangeCount) return;

  // Check if selection/cursor is before the last prompt
  const range = sel.getRangeAt(0);
  const terminal = document.getElementById('repl-terminal');

  // Walk through nodes to see if cursor is before last prompt
  let beforePrompt = true;
  let node = terminal.firstChild;

  while (node) {
    if (node === lastPrompt) {
      beforePrompt = false;
    }
    if (node === range.startContainer || node.contains(range.startContainer)) {
      if (beforePrompt) {
        // Cursor is before last prompt, move it to end
        e.preventDefault();
        moveCursorToEnd(terminal);
        return false;
      }
      break;
    }
    node = node.nextSibling;
  }

  return true;
}

// Handle keydown in terminal
function handleTerminalKeydown(e) {
  const terminal = document.getElementById('repl-terminal');
  const lastPrompt = getLastPrompt();

  if (e.metaKey && e.key === 'Enter') {
    e.preventDefault();
    executeCurrentLine();
    return;
  }

  // Prevent backspace/delete from removing history
  if (e.key === 'Backspace' || e.key === 'Delete') {
    const currentInput = getCurrentInput();

    // If no input and backspace, prevent default
    if (currentInput.length === 0 && e.key === 'Backspace') {
      e.preventDefault();
      return;
    }
  }

  // For arrow keys and other navigation, check if we're in editable area
  if (['ArrowLeft', 'ArrowUp', 'Home'].includes(e.key)) {
    setTimeout(() => {
      const sel = window.getSelection();
      if (!sel.rangeCount) return;

      const range = sel.getRangeAt(0);
      let node = range.startContainer;

      // Check if cursor moved before the last prompt
      let beforePrompt = true;
      let current = terminal.firstChild;

      while (current) {
        if (current === lastPrompt) {
          beforePrompt = false;
        }
        if (current === node || current.contains(node)) {
          if (beforePrompt) {
            // Move cursor to start of editable area (after prompt)
            const newRange = document.createRange();
            const newSel = window.getSelection();

            if (lastPrompt.nextSibling) {
              newRange.setStart(lastPrompt.nextSibling, 0);
            } else {
              newRange.setStartAfter(lastPrompt);
            }
            newRange.collapse(true);
            newSel.removeAllRanges();
            newSel.addRange(newRange);
          }
          break;
        }
        current = current.nextSibling;
      }
    }, 0);
  }
}

// Prevent editing before last prompt
function handleBeforeInput(e) {
  if (!isInEditableArea()) {
    e.preventDefault();
    moveCursorToEnd(document.getElementById('repl-terminal'));
  }
}

// Handle paste to ensure it goes in editable area
function handlePaste(e) {
  if (!isInEditableArea()) {
    e.preventDefault();
    moveCursorToEnd(document.getElementById('repl-terminal'));
  }
}

// Handle click to enforce editable area
function handleTerminalClick(e) {
  setTimeout(() => {
    const lastPrompt = getLastPrompt();
    if (!lastPrompt) return;

    const sel = window.getSelection();
    if (!sel.rangeCount) return;

    const range = sel.getRangeAt(0);
    const terminal = document.getElementById('repl-terminal');

    // Check if click was before last prompt
    let beforePrompt = true;
    let node = terminal.firstChild;

    while (node) {
      if (node === lastPrompt) {
        beforePrompt = false;
        break;
      }
      if (node === range.startContainer || node.contains(range.startContainer)) {
        if (beforePrompt) {
          moveCursorToEnd(terminal);
        }
        return;
      }
      node = node.nextSibling;
    }
  }, 0);
}

// Clear REPL
function clearREPL() {
  const terminal = document.getElementById('repl-terminal');
  terminal.innerHTML = '';
  terminal.appendChild(createPrompt());
  moveCursorToEnd(terminal);

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
    terminal.addEventListener('beforeinput', handleBeforeInput);
    terminal.addEventListener('paste', handlePaste);
    terminal.addEventListener('click', handleTerminalClick);
    initTerminal();
  }
}

// Expose functions to window for onclick handlers
window.clearREPL = clearREPL;

// Debug helper: dump the MIR for a YLC source string to the console.
// Usage in the browser console: ylcDumpMir("let x = 1 in x")
// The MIR text is written to wasi stdout, which the shim routes to
// console.log("[stdout]", ...) and to the terminal element.
window.ylcDumpMir = (source) => {
  if (!wasmInstance) {
    console.error('WASM not loaded yet');
    return;
  }
  const ptr = repl.writeString(source);
  const status = wasmInstance.exports.ylc_wasm_dump_mir(ptr);
  wasmInstance.exports.free(ptr);
  if (status !== 0) {
    console.error('ylc_wasm_dump_mir failed (status ' + status + ')');
  }
  return status;
};

// Load WASM on page load
window.addEventListener('load', () => {
  loadWasm();
  setupTerminal();
});
