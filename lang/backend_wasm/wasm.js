import { WASI, OpenFile, ConsoleStdout, File } from "https://cdn.jsdelivr.net/npm/@bjorn3/browser_wasi_shim@0.3.0/dist/index.js";

let wasmInstance = null;
let wasmMemory = null;
let context = null;
let wasi = null;
let repl = null;

// REPL for incremental WASM execution
class YLCWasmREPL {
  constructor(wasmInstance, wasmMemory, context, baseModule) {
    this.wasmInstance = wasmInstance;
    this.wasmMemory = wasmMemory;
    this.context = context;
    this.baseModule = baseModule; // Base module with shared table/memory
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
    // const fullProgram = this.getFullProgram();
    const fullProgram = this.codeHistory[this.codeHistory.length - 1];

    if (!fullProgram.trim()) {
      return null;
    }

    console.log(fullProgram);

    try {
      // Parse, typecheck, and generate executable WASM module
      const inputPtr = this.writeString(fullProgram);
      const moduleSizePtr = this.wasmInstance.exports.malloc(4);
      const modulePtr = this.wasmInstance.exports.generate_executable_module(
        this.context, inputPtr, moduleSizePtr
      );

      if (!modulePtr || modulePtr === 0) {
        this.wasmInstance.exports.free(inputPtr);
        this.wasmInstance.exports.free(moduleSizePtr);
        throw new Error("Parse, type error, or code generation failed");
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

      // Instantiate the generated module with base module imports
      const imports = this.createImports();
      const { instance } = await WebAssembly.instantiate(moduleCopy, imports);

      this.lastModule = instance;

      // Execute the eval function
      if (instance.exports.eval) {
        const result = instance.exports.eval();
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
        // Import shared resources from base module
        table: this.baseModule.exports.table,
        memory: this.baseModule.exports.memory,
        store_value: this.baseModule.exports.store_value,
        load_value: this.baseModule.exports.load_value,
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
    if (wasmInstance.exports.create_wasm_context) {
      context = wasmInstance.exports.create_wasm_context();
      console.log('Created context:', context);
    }

    // Generate and instantiate base module
    console.log('Generating base module...');
    const baseSizePtr = wasmInstance.exports.malloc(4);
    const baseModulePtr = wasmInstance.exports.generate_base_module(baseSizePtr);

    if (!baseModulePtr || baseModulePtr === 0) {
      wasmInstance.exports.free(baseSizePtr);
      throw new Error("Failed to generate base module");
    }

    const dataView = new DataView(wasmMemory.buffer);
    const baseModuleSize = dataView.getUint32(baseSizePtr, true);
    console.log('Base module size:', baseModuleSize);
    const baseModuleBytes = new Uint8Array(wasmMemory.buffer, baseModulePtr, baseModuleSize);
    const baseModuleCopy = new Uint8Array(baseModuleBytes);

    // Debug: print first 30 bytes
    // console.log('Base module first 30 bytes:', Array.from(baseModuleCopy.slice(0, 50)).map(b => '0x' + b.toString(16).padStart(2, '0')).join(' '));

    wasmInstance.exports.free(baseModulePtr);
    wasmInstance.exports.free(baseSizePtr);

    console.log('Instantiating base module...');
    const { instance: baseModule } = await WebAssembly.instantiate(baseModuleCopy, {});
    console.log('Base module loaded:', Object.keys(baseModule.exports));

    // Initialize REPL with base module
    repl = new YLCWasmREPL(wasmInstance, wasmMemory, context, baseModule);
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

    // Add code to REPL history
    console.log(`execute '${input}'`)
    repl.addCode(input);

    // Execute
    const result = await repl.execute();

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

    // Remove last code from history on error
    repl.codeHistory.pop();

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

// Load WASM on page load
window.addEventListener('load', () => {
  loadWasm();
  setupTerminal();
});
