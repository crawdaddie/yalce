import { init, WASI } from "@wasmer/wasi";
await init();

let wasi = new WASI({
  env: {
    // 'ENVVAR1': '1',
    // 'ENVVAR2': '2'
  },
  args: [
    // 'command', 'arg1', 'arg2'
  ],
});

const moduleBytes = fetch("jit.wasm");
const module = await WebAssembly.compileStreaming(moduleBytes);


const instance = wasi.instantiate(module, {
  wasi_snapshot_preview1: {
    sock_accept() {
      return 0;
    }
  }
});


// console.log(wasi);

const textarea = document.getElementById('expression');

async function onTestChange(event) {

  if (event.key === 'Enter' && !event.shiftKey) {
    event.preventDefault();
    const lines = textarea.value.trim().split('\n');
    const lastLine = lines[lines.length - 1];
    lockTextArea(true);

    let resultText = "\n";

    try {
      // Simple expression evaluation
      // const [a, b] = lastLine.split('+').map(n => parseInt(n.trim(), 10));
      // if (!isNaN(a) && !isNaN(b)) {
      //   const sum = instance.exports.add(a, b);
      //   resultText = `\n> Result: ${sum}\n`;
      // } else {
      //   resultText = '\n> Invalid input. Please enter two numbers separated by +\n';
      // }
      let res = instance.exports.jit(lastLine);
      console.log(res)
    } catch (error) {
      console.error(error);
      resultText = `\n> Error: ${error.message}\n`;

    }

    textarea.value += resultText;
    lockTextArea(false);
  }
}

function lockTextArea(lock) {
  textarea.readOnly = lock;
  textarea.classList.toggle('locked', lock);
}

textarea.addEventListener('keydown', onTestChange);
