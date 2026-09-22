# MNIST Implementation Guide for YLC Autograd

## Part A: Theory

### What MNIST requires that binary classification doesn't

**1. Multi-class classification (10 classes)**
- Binary: sigmoid output → BCE loss
- Multi-class: softmax output → cross-entropy loss
- Softmax: `softmax(z_i) = exp(z_i) / Σ_j exp(z_j)` — normalizes 10 logits into probabilities that sum to 1
- Cross-entropy: `CE = -Σ_k y_k * log(p_k)` where `y_k` is the one-hot label (1 for correct class, 0 otherwise)

**2. Per-row reduction (the key missing op)**
- For a batch of `[B, 10]` logits, softmax needs `Σ_j exp(z_ij)` **per row** → `[B, 1]`
- The original `tensor_sum` reduces **everything** to `[1, 1]` — a global sum, not per-row
- `tensor_sum_rows`: `[B, 10] → [B, 1]` (sum along axis 1, keep axis 0) — now implemented

**3. One-hot encoding**
- Label "3" becomes `[0, 0, 0, 1, 0, 0, 0, 0, 0, 0]`
- Training target: `[B, 10]` one-hot matrix

**4. Larger model architecture**
- Input: 784 (28×28 flattened)
- Hidden: 128 → 64 (ReLU)
- Output: 10 (softmax)
- Matmuls: `[B, 784] × [784, 128] → [B, 128]` etc.

**5. Evaluation: argmax + accuracy**
- `prediction = argmax(softmax_output)` — pick the class with highest probability
- `accuracy = correct / total`

### Learning resources
- **3Blue1Brown "Neural Networks" series** — visual intuition for softmax, cross-entropy, backprop
- **Michael Nielsen's "Neural Networks and Deep Learning"** — free online book, uses MNIST throughout, builds a network from scratch in Python
- **Stanford CS231n notes** — softmax classifier, cross-entropy gradient derivation

---

## Part B: What's implemented in `autograd.ylc`

| Feature | Status | Purpose |
|---------|--------|---------|
| `tensor_sum_rows` | ✅ | `[B, C] → [B, 1]` per-row sum (softmax normalization) |
| `softmax` | ✅ | Numerically stable (subtract max per row), differentiable through exp/sum/div |
| `tensor_argmax` | ✅ | Pick highest-probability class (evaluation only, no grad) |
| `tensor_he_init` | ✅ | He initialization for ReLU layers: `scale = sqrt(2 / fan_in)` |
| `tensor_xavier_init` | ✅ | Xavier init for sigmoid/tanh: `scale = sqrt(1 / fan_in)` |
| `adam_step` | ✅ | Adam optimizer (flat m/v/t buffers + offsets) |
| `adam_create_m/v/t` | ✅ | Create Adam state arrays |
| `adam_offsets` | ✅ | Prefix-sum offsets for flat buffer indexing |
| `clip_grads` | ✅ | Gradient clipping by global L2 norm |
| `cos_lr` | ✅ | Cosine learning rate decay schedule |
| `tensor_matmul` | ✅ | `[B, in] × [in, out] → [B, out]` |
| Row-bias broadcast | ✅ | `[1, C] + [B, C] → [B, C]` (bias broadcast) |
| `tensor_relu` | ✅ | Elementwise ReLU with backward |
| `tensor_exp/log` | ✅ | Elementwise with backward |
| `tensor_neg/div/mul/sq` | ✅ | Elementwise with backward |
| `tensor_sum` | ✅ | Global sum to `[1, 1]` (for BCE loss) |
| `linear` | ✅ | `matmul x w + b` with row-bias broadcast |

---

## Part C: What's NOT yet implemented

### 1. File I/O: loading MNIST data

MNIST comes in IDX binary format:
- `train-images-idx3-ubyte`: 60,000 images, 28×28 bytes each (0-255 grayscale)
- `train-labels-idx1-ubyte`: 60,000 labels (0-9)
- `t10k-images-idx3-ubyte`: 10,000 test images
- `t10k-labels-idx1-ubyte`: 10,000 test labels

**Options:**
- **C extern**: write a `load_mnist` function in C that reads IDX files and returns `_DoubleArray` (the YLC array type). Register via `extern fn`.
- **Pre-convert to CSV**: convert MNIST to `label,pixel1,pixel2,...,pixel784` format and read line-by-line using existing `fopen`/`fread` externs.
- **Hardcode a small subset**: for testing, embed a few hundred samples as array literals (impractical for 60K but fine for prototyping).

**C helper signature (proposed):**
```c
// Returns flattened images as _DoubleArray [n, 784], normalized to [0, 1]
_DoubleArray load_mnist_images(const char *path, int32_t n);

// Returns one-hot labels as _DoubleArray [n, 10]
_DoubleArray load_mnist_labels(const char *path, int32_t n);
```

### 2. Cross-entropy loss (multi-class)

Build from existing ops — no new ops needed:
```ylc
let cross_entropy = fn pred target ->
  let log_p = tensor_log pred;           # [B, 10]
  let prod = tensor_mul target log_p;     # [B, 10] (one-hot * log_prob)
  let per_sample = tensor_sum_rows prod; # [B, 1] (sum over classes)
  tensor_neg per_sample                   # [B, 1] (negative = loss per sample)
;;
```

For mean CE over batch, divide by batch size:
```ylc
let cross_entropy_mean = fn pred target ->
  let ce = cross_entropy pred target;
  let batch_size = (tensor_shape pred)[0] in
  let batch_d = tensor [| 1, 1 |] [| (Double batch_size) |] in
  tensor_div ce batch_d
;;
```

### 3. One-hot encoding

```ylc
let one_hot = fn label num_classes ->
  let out = array_fill_const num_classes 0. in (
    out[label] := 1.;
    out
  )
;;

# Build the full target matrix at data loading time:
# labels = [0, 3, 1, 9, ...] → targets = [[1,0,0,...], [0,0,0,1,0,...], ...]
```

### 4. Data normalization

Pixels are 0-255. Normalize to [0, 1] or [-1, 1]:
```ylc
# In the C loader, divide each pixel by 255.0
# Or in YLC after loading:
for i = 0 .. array_size pixel_data in (pixel_data[i] := pixel_data[i] / 255.)
```

### 5. Mini-batch iteration

Slice the training data into batches:
```ylc
let BATCH_SIZE = 64;
let NUM_TRAIN = 60000;
let num_batches = NUM_TRAIN / BATCH_SIZE;

for batch_idx = 0 .. num_batches in (
  let start = batch_idx * BATCH_SIZE;
  let end = start + BATCH_SIZE;
  let x_batch = tensor [| BATCH_SIZE, 784 |] (train_x_data[start * 784 .. end * 784]);
  let y_batch = tensor [| BATCH_SIZE, 10 |] (train_y_data[start * 10 .. end * 10]);
  # ... train on batch
)
```

---

## Part D: Example MNIST script structure

```ylc
let _ = dlopen "libs/autograd/libautograd.so";
open libs/autograd/autograd;
import std/Math;

# ---- Load data (needs C helper or pre-converted CSV) ----
# let train_x = load_mnist_images "train-images-idx3-ubyte";  # [60000, 784]
# let train_y = load_mnist_labels "train-labels-idx1-ubyte";  # [60000, 10] one-hot
# let test_x  = load_mnist_images "t10k-images-idx3-ubyte";    # [10000, 784]
# let test_y  = load_mnist_labels "t10k-labels-idx1-ubyte";   # [10000, 10] one-hot

# ---- Model: 784 → 128 → 64 → 10 ----
let Model = module () ->
  let w1 = tensor_he_init [| 784, 128 |];
  let b1 = tensor_zeroes [| 1, 128 |];
  let w2 = tensor_he_init [| 128, 64 |];
  let b2 = tensor_zeroes [| 1, 64 |];
  let w3 = tensor_he_init [| 64, 10 |];
  let b3 = tensor_zeroes [| 1, 10 |];
  let params = [| w1, b1, w2, b2, w3, b3 |];

  let model = fn x ->
    let h1 = tensor_relu (linear w1 b1 x);
    let h2 = tensor_relu (linear w2 b2 h1);
    softmax (linear w3 b3 h2)
  ;;

  let infer = fn x -> model x;;

  let zero_grads = fn () ->
    for i = 0 .. array_size params in (tensor_zero_grad (params[i]))
  ;;
;;

# ---- Cross-entropy loss ----
let cross_entropy = fn pred target ->
  let log_p = tensor_log pred;
  let prod = tensor_mul target log_p;
  let per_sample = tensor_sum_rows prod;
  tensor_neg per_sample
;;

let cross_entropy_mean = fn pred target ->
  let ce = cross_entropy pred target;
  let batch_size = (tensor_shape pred)[0] in
  let batch_d = tensor [| 1, 1 |] [| (Double batch_size) |] in
  tensor_div ce batch_d
;;

# ---- Adam optimizer state ----
let offsets = adam_offsets Model.params;
let m_buf = adam_create_m Model.params;
let v_buf = adam_create_v Model.params;
let t_arr = adam_create_t Model.params;

# ---- Training loop ----
let BATCH_SIZE = 64;
let EPOCHS = 10;
let LR = 0.001;
let LAMBDA = 0.0001;
let NUM_TRAIN = 60000;
let num_batches = NUM_TRAIN / BATCH_SIZE;

for epoch = 0 .. EPOCHS in (
  let lr = cos_lr epoch LR (LR * 0.01) EPOCHS;
  for batch = 0 .. num_batches in (
    let start = batch * BATCH_SIZE;
    let end = start + BATCH_SIZE;
    # let x_batch = tensor [| BATCH_SIZE, 784 |] (train_x[start * 784 .. end * 784]);
    # let y_batch = tensor [| BATCH_SIZE, 10 |] (train_y[start * 10 .. end * 10]);
    Model.zero_grads ();
    # let pred = Model.infer x_batch;
    # let loss = cross_entropy_mean pred y_batch;
    # tensor_backward_loss loss;
    # clip_grads Model.params 1.0;
    # adam_step Model.params m_buf v_buf t_arr offsets lr LAMBDA
  )
);

# ---- Evaluation ----
# let pred = Model.infer test_x;
# let predicted = tensor_argmax pred;   # [10000] class indices
# let correct = count_correct predicted test_labels;
# print `accuracy: {correct / 10000.0}\n`;

# ---- Graph compilation (flatten to SSA for CUDA codegen) ----
# let fg, output_id = Model.infer (tensor [| 1, 784 |] test_x_data[0..784])
#   |> graph_compile.flatten_graph;
# print_flat_nodes fg;
```

---

## Part E: Performance estimates

MNIST with `784 → 128 → 64 → 10` at `batch_size=64`:

| Metric | Value |
|--------|-------|
| Forward FLOPs per batch | 784×128 + 128×64 + 64×10 ≈ 108K multiply-adds |
| Batches per epoch | 60000 / 64 ≈ 937 |
| Epochs | 10 |
| Total batches | 9,370 |
| Total FLOPs | ~1 billion |
| Current speed (scalar JIT) | ~70 MFLOPS |
| Est. time per epoch | ~14s |
| Est. total training time | ~140s |
| With BLAS matmul | ~14s total |
| With CUDA codegen | ~1-2s total |

The flattened SSA graph for this model has ~25 nodes — directly mappable to CUDA kernels for the GPU codegen pipeline.

---

## Part F: CUDA codegen path

The flattened SSA graph from `graph_compile.flatten_graph` produces `List of FlatNode`, each with:
- `id` — SSA value number
- `op` — operation type (Matmul, Add, Relu, Exp, Div, etc.)
- `shape` — `[rows, cols]`
- `operands` — input SSA ids

For MNIST, the graph would be:
```
%0  = input leaf [B, 784]
%1  = w1 leaf [784, 128]
%2  = matmul %0 %1          → [B, 128]
%3  = add b1 %2              → [B, 128]   (bias broadcast)
%4  = relu %3                → [B, 128]
%5  = w2 leaf [128, 64]
%6  = matmul %4 %5           → [B, 64]
%7  = add b2 %6              → [B, 64]
%8  = relu %7                → [B, 64]
%9  = w3 leaf [64, 10]
%10 = matmul %8 %9           → [B, 10]
%11 = add b3 %10             → [B, 10]
# softmax (exp, sum_rows, div) adds ~5 more nodes
%16 = softmax output         → [B, 10]
```

Each node maps to one CUDA kernel:
- **Matmul**: `cuda_matmul` — BLAS or tiled kernel
- **Add (bias)**: `cuda_elementwise_add_broadcast` — one thread per element
- **ReLU**: `cuda_elementwise_relu` — one thread per element
- **Exp/Div**: `cuda_elementwise` — one thread per element
- **Sum_rows**: `cuda_reduce_rows` — warp shuffle reduction per row

The backward pass is the reverse of the SSA list, with each op's backward kernel accumulating into operand grad buffers.

---

## Part G: Implementation checklist

- [x] `tensor_sum_rows` — per-row sum for softmax
- [x] `softmax` — numerically stable, differentiable
- [x] `tensor_argmax` — evaluation metric
- [x] `tensor_he_init` — He initialization
- [x] `adam_step` + `adam_create_m/v/t` + `adam_offsets` — Adam optimizer
- [x] `clip_grads` — gradient clipping
- [x] `cos_lr` — cosine LR decay
- [ ] C helper to load MNIST IDX files
- [ ] `cross_entropy` loss function (copy from Part D)
- [ ] One-hot encoding helper
- [ ] Mini-batch data slicing
- [ ] Full MNIST training script
- [ ] CUDA codegen from flattened SSA graph
