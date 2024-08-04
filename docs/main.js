const $ = document.getElementById.bind(document);
const editor = $('expression');

let editorOffset = 0;

function throw_error() { throw new Error; }

const defaultHandler = (name) => (...args) => {
  console.log(`Called ${name} with args:`, args);
  return 0;
};

let print = console.log;

function pushToEditor(text) {
  editor.innerHTML += `<span contenteditable="false" class="jit-output">  ${text}</span>`;
  editorOffset = editor.textContent.length;
}

// WASI polyfill that's enough to implement fwrite(stdout, "foo");
const Handlers = {
  fd_close(fd) {
    print(`closed ${fd}`);
    return 0;
  },
  fd_fdstat_get(fd, fdstat) {
    if (fd != 1 && fd != 2)
      return -1;
    // struct __wasi_fdstat_t {
    //   uint8_t filetype;
    //   uint16_t flags;
    //   uint64_t rights_base;
    //   uint64_t rights_inheriting;
    // };
    let buf = new Uint8Array(instance.exports.memory.buffer, fdstat,
                             24); // sizeof __wasi_fdstat_t;
    buf[0] = 2; // __WASI_FILETYPE_CHARACTER_DEVICE
    buf[1] = 0;
    let i;
    for (i = 2; i < 4; i++)
      buf[i] = 0; // No flags.
    for (i = 4; i < 8; i++)
      buf[i] = 0;
    for (i = 8; i < 24; i++)
      buf[i] = 0; // Clear rights bitmaps.
    return 0;
  },
  fd_seek(fd, offset, whence, size_out) {
    // Seems to be unused.
    print(`seek ${fd}, ${offset}, ${whence}, ${size_out}`);
    return 0;
  },
  fd_write(fd, iov, iocount, error) {
    let out = '';
    iov = new Uint32Array(instance.exports.memory.buffer, iov, iocount * 2);
    for (let i = 0; i < iocount; i++) {
      let ptr = iov[i * 2]
      let len = iov[i * 2 + 1]
      let bytes = new Uint8Array(instance.exports.memory.buffer, ptr, len);
      for (let b of bytes)
        out += String.fromCharCode(b);
    }

    if (fd === 1) {
      pushToEditor(out);
    }
    print(out);
    return out.length;
  },
};

let wasi_snapshot_preview1 = new Proxy({}, {
  get(target, prop) {
    return Handlers[prop] || defaultHandler(prop);
  }
});  

let imports = {
  env: { throw_error },
  wasi_snapshot_preview1
}; 


let { mod, instance } = await WebAssembly.instantiateStreaming(
  fetch("jit.wasm", {credentials:"same-origin"}),
  imports
);
instance.exports._initialize();

function writeString(str) {
  let len = str.length + 1;
  let ptr = instance.exports.allocateBytes(len);
  let buf = new Uint8Array(instance.exports.memory.buffer, ptr, len);
  let i = 0;
  for (let c of str) {
    let code = c.codePointAt(0);
    if (code > 127)
      throw new Error("ascii only, please");
    buf[i++] = code;
  }
  buf[i] = 0;
  return ptr;
}

function parse(str) {
  let chars = writeString(str);
  let expr = instance.exports.parse(chars);
  // instance.exports.freeBytes(chars);
  return expr;
}

function _eval(expr) {
  return instance.exports.eval(expr, 1024 * 1024);
}

let alreadyJitted = [];
function jit() {
  let ptr = instance.exports.jitModule()
  if (!ptr) {
    print('No pending JIT code.', 'p');
    return;
  }
  let data = instance.exports.moduleData(ptr);
  let size = instance.exports.moduleSize(ptr);
  print(`Got ${size} bytes of JIT code.  Patching code into interpreter.`, 'p');
  let memory = instance.exports.memory;
  let __indirect_function_table = instance.exports.__indirect_function_table;
  let bytes = memory.buffer.slice(data, data + size);
  instance.exports.freeModule(ptr)
  let mod = new WebAssembly.Module(bytes);
  let env = {throw_error, memory, __indirect_function_table};
  let imports = {env};
  new WebAssembly.Instance(mod, imports);
  for (let i = 0; i < defCount; i++) {
    alreadyJitted[i] = true;
  }
}

let escape = str => {
  return str.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
};

let defCount = 0;

let ignoredDefinitions = new Set();

function allocateString(instance, str) {
  const encoder = new TextEncoder();
  const bytes = encoder.encode(str + '\0'); // null-terminated string
  const ptr = instance.exports.malloc(bytes.length);
  const memory = instance.exports.memory.buffer;
  new Uint8Array(memory).set(bytes, ptr);
  return ptr;
}

function freeString(instance, ptr) {
  instance.exports.free(ptr);
}

function appendLambda() {
  editor.innerHTML += '<span contenteditable="false" class="lambda">λ </span>';
  editorOffset = editor.textContent.length;
}

function appendNL() {
  editor.innerHTML += '<br>';
  editorOffset = editor.textContent.length;
}

function printBytes(uint8Array) {
  for (let i = 0; i < uint8Array.length; i++) {
    let hex = uint8Array[i].toString(16).padStart(2, '0');
    console.log(`0x${hex}`);
  }
}
async function onTextChange(event) {
  if (event.key === 'Enter' && !event.shiftKey) {
    event.preventDefault();
    const content = editor.innerHTML;
    const lastInput = content.slice(content.lastIndexOf('λ </span>') + 9);
    lockEditor(true);
    appendNL();


    const {jit, module_size, module_data, memory } = instance.exports;

    try {
      // Allocate memory for the input string
      const inputPtr = allocateString(instance, lastInput);
      const modulePtr  = jit(inputPtr);
      const size = module_size(modulePtr);
      const data = module_data(modulePtr);
      let bytes = new Uint8Array(memory.buffer.slice(data, data + size));

      let newMod = await WebAssembly.instantiate(
        bytes,
        imports
      );

      const result = newMod.instance.exports.main();
      console.log('Result:', result);
      pushToEditor(`${result}\n`);

      freeString(instance, inputPtr);
    } catch (error) {
      console.error(error);
      pushToEditor(`<br>> Error: ${error.message}<br>`);
    }

    appendLambda();
    lockEditor(false);

    // Set cursor to the end
    setCaretToEnd(editor);
  }
  if (event.key === 'Backspace' && editor.textContent.length <= editorOffset) {
    event.preventDefault();
  }
}

function lockEditor(lock) {
  editor.contentEditable = !lock;
  editor.classList.toggle('locked', lock);
}

function setCaretToEnd(element) {
  const range = document.createRange();
  const selection = window.getSelection();
  range.selectNodeContents(element);
  range.collapse(false);
  selection.removeAllRanges();
  selection.addRange(range);
  element.focus();
}

editor.addEventListener('keydown', onTextChange);

appendLambda();
editorOffset = editor.textContent.length;

setCaretToEnd(editor);
