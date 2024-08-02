import { init, WASI } from "@wasmer/wasi";

await init();
// Create a WebAssembly Memory instance
// const memory = new WebAssembly.Memory({ initial: 2 }); // Size is in pages (64KB each)

let wasi = new WASI({
  env: {
    // 'ENVVAR1': '1',
    // 'ENVVAR2': '2'
    //
  },
  args: [
    // 'command', 'arg1', 'arg2'
  ],
});

const moduleBytes = fetch("assets/jit.wasm");
const module = await WebAssembly.compileStreaming(moduleBytes);

const instance = wasi.instantiate(module, {

  wasi_snapshot_preview1: {
    sock_accept() {
      return 0;
    }
  },
});

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

const textarea = document.getElementById('expression');

async function onTestChange(event) {

  if (event.key === 'Enter' && !event.shiftKey) {
    event.preventDefault();
    const lines = textarea.value.trim().split('\n');
    const lastLine = lines[lines.length - 1];
    lockTextArea(true);

    let resultText = "\n";

    try {
      // Allocate memory for the input string
      const inputPtr = allocateString(instance, lastLine);
      let res = instance.exports.jit(inputPtr);
      const stdout = wasi.getStdoutString();
      console.log(stdout);

      resultText = `\n> ${stdout}`;

      freeString(instance, inputPtr);
    } catch (error) {
      console.error(error);
      resultText = `\n> Error: ${error.message}\n`;
    }

    textarea.value += resultText;
    textarea.value += "λ ";
    lockTextArea(false);
  }
}

function lockTextArea(lock) {
  textarea.readOnly = lock;
  textarea.classList.toggle('locked', lock);
}

textarea.addEventListener('keydown', onTestChange);
