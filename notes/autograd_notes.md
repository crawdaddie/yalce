• A good first forward-pass target is a tiny linear regression loss using only ops you already have:

  pred = tensor_add (tensor_matmul x w) b
  err  = tensor_sub pred target
  sq   = tensor_mul err err
  loss = tensor_sum sq

  Avoid broadcasting at first. Make b the same shape as pred, even if that is less elegant, because it keeps shape and backward rules simple.

  Forward Pass Steps

  1. Define shape conventions:
      - x: [| batch, in_features |]
      - w: [| in_features, out_features |]
      - b: [| batch, out_features |]
      - target: [| batch, out_features |]
      - loss: [| 1 |]

  2. Add small model helpers:
      - linear = fn x w b -> tensor_add (tensor_matmul x w) b
      - mse_raw = fn pred target -> tensor_sum (tensor_mul err err) where err = tensor_sub pred target
      - network_loss = fn x w b target -> mse_raw (linear x w b) target

  3. Add runtime tests for one concrete case:
      - choose tiny shapes like x: 1x2, w: 2x1, b: 1x1
      - verify matmul, add, sub, mul, sum produce the expected scalar loss
      - verify the top-level op of loss is TensOpSum

  4. Do not mutate data during forward.
     Each op should allocate a new TensorNode, store computed data, zero grad, and record the parent refs through op.

  5. Add shape checks later.
     Initially, tests are enough. Later you probably want helpers like tensor_rows, tensor_cols, same_shape, and maybe fail/print on mismatch.

  Backward Pass Steps

  1. Seed the output gradient:
      - for scalar loss, set loss.grad[0] := 1.
      - this means d loss / d loss = 1

  2. Traverse the graph from loss.
     Since each node stores parents in op, you can recursively walk:
      - visit children first to build topological order, then process reverse order
      - or recursively call backward after accumulating parent grads
      - eventually you will want a visited set or node id, but for small examples recursion is fine

  3. Gradients must accumulate, not replace.
     If one tensor contributes to multiple downstream paths, do:

  parent.grad[i] := parent.grad[i] + contribution[i]

  4. Implement gradient rules per op:

  Add:
    z = a + b
    da += dz
    db += dz

  Sub:
    z = a - b
    da += dz
    db += -dz

  Neg:
    z = -a
    da += -dz

  Mul:
    z = a * b
    da += dz * b.data
    db += dz * a.data

  Div:
    z = a / b
    da += dz / b.data
    db += -(dz * a.data) / (b.data * b.data)

  Sum:
    z = sum(a)
    da[i] += dz[0]

  MatMul:
    z = a @ b
    da += dz @ transpose(b)
    db += transpose(a) @ dz

  5. Add missing helpers before full backward:
      - tensor_zero_grad
      - tensor_add_grad
      - array_add_inplace
      - maybe transpose_2d or manual matmul-gradient loops

  6. Start backward with only Sum, Mul, Sub, and Add.
     That is enough for:

  loss = tensor_sum (tensor_mul err err)

  Then add MatMul once scalar loss gradients work for simple elementwise expressions.

  7. Test gradients numerically.
     For the simple loss:

  loss = sum((pred - target)^2)

  The expected gradient wrt pred is:

  2 * (pred - target)

  So before testing w and b, first check that err/pred receives the expected gradient. Then test matmul parameter gradients.
