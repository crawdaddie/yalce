console.log("hello");
const $ = document.getElementById.bind(document);
const textarea = $('expression');

let textareaOffset = 0;

async function load_wasm_jit_demo() {
  function throw_error() { throw new Error; }

  const defaultHandler = (name) => (...args) => {
    console.log(`Called ${name} with args:`, args);
    return 0;
  };

  let print = console.log;

  function pushToTextArea(text) {
      textarea.value += text;
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
        pushToTextArea(out);
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

  async function onTextChange(event) {

    if (event.key === 'Enter' && !event.shiftKey) {
      event.preventDefault();
      const lastInput = textarea.value.slice(textareaOffset);
      lockTextArea(true);


      textarea.value += "\n";

      try {
        // Allocate memory for the input string
        const inputPtr = allocateString(instance, lastInput);
        instance.exports.jit(inputPtr);
        freeString(instance, inputPtr);
      } catch (error) {
        console.error(error);
        resultText = `\n> Error: ${error.message}\n`;
      }

      textarea.value += "λ ";
      textareaOffset = textarea.value.length;
      lockTextArea(false);
    }
    if (event.key === 'Backspace' && textarea.value.length == textareaOffset) {
      event.preventDefault();
    }
  }

  function lockTextArea(lock) {
    textarea.readOnly = lock;
    textarea.classList.toggle('locked', lock);
  }
  textarea.addEventListener('keydown', onTextChange);
}

load_wasm_jit_demo();
