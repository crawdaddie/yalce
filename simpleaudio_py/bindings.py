import threading
import ctypes
import time


# Define the structure in Python with the same layout as the C struct
class CNode(ctypes.Structure):
    pass


def node_ptr(res):
    return ctypes.cast(res, ctypes.POINTER(CNode))


lib = ctypes.CDLL('libsimpleaudio.so')

# Declare the C functions
lib.sin_node.restype = ctypes.POINTER(CNode)
lib.impulse_node.restype = ctypes.POINTER(CNode)
lib.poly_saw_node.restype = ctypes.POINTER(CNode)
lib.sq_node.restype = ctypes.POINTER(CNode)
lib.sq_detune_node.restype = ctypes.POINTER(CNode)
lib.pulse_node.restype = ctypes.POINTER(CNode)

lib.container_node.argtypes = [ctypes.POINTER(CNode)]
lib.container_node.restype = ctypes.POINTER(CNode)
lib.ctx_add_after_tail.argtypes = [ctypes.POINTER(CNode)]

set_double_argtype = [
    ctypes.POINTER(CNode), ctypes.c_int, ctypes.c_double]

set_node_argtype = [
    ctypes.POINTER(CNode), ctypes.c_int, ctypes.POINTER(CNode)]

lib.node_set_sig_double.argtypes = [
    ctypes.POINTER(CNode), ctypes.c_int, ctypes.c_double]

lib.node_set_sig_double_lag.argtypes = [
    ctypes.POINTER(CNode), ctypes.c_int, ctypes.c_double, ctypes.c_double]

lib.node_set_sig_node.argtypes = [
    ctypes.POINTER(CNode), ctypes.c_int, ctypes.POINTER(CNode)]

lib.node_set_add_node.argtypes = [
    ctypes.POINTER(CNode), ctypes.POINTER(CNode)]

lib.node_set_add_double.argtypes = [ctypes.POINTER(CNode), ctypes.c_double]
lib.node_set_mul_node.argtypes = [
    ctypes.POINTER(CNode), ctypes.POINTER(CNode)]

lib.node_set_mul_double.argtypes = [ctypes.POINTER(CNode), ctypes.c_double]


def get_setter(val):
    if isinstance(val, Node):
        return lib.node_set_sig_node

    return lib.node_set_sig_double


def get_mul_setter(val):
    if isinstance(val, Node):
        return lib.node_set_mul_node

    return lib.node_set_mul_double


def get_add_setter(val):
    if isinstance(val, Node):
        return lib.node_set_add_node

    return lib.node_set_add_double


def node_set_lag(node, idx, val, lag=1.0):
    lib.node_set_sig_double_lag(node.ptr, idx, val, lag)


def node_set(node, idx, val):
    if isinstance(val, Node):
        lib.node_set_sig_node(node.ptr, idx, val.ptr)
        return node

    lib.node_set_sig_double(node.ptr, idx, val)


def node_set_mul(node, val):
    if isinstance(val, Node):
        lib.node_set_mul_node(node.ptr, val.ptr)
        return node
    lib.node_set_mul_double(node.ptr, val)


def node_set_add(node, val):
    if isinstance(val, Node):
        lib.node_set_add_node(node.ptr, val.ptr)
        return node
    lib.node_set_add_double(node.ptr, val)


class Node():
    def __init__(self, ptrs, mul=1, add=0):
        self.ptr = node_ptr(ptrs[0])
        self.mul = mul
        self.add = add

    def container(self):
        container_ptr = lib.container_node(self.ptr)
        return node_ptr(container_ptr)

    @property
    def mul(self):
        return self._mul

    @mul.setter
    def mul(self, value):
        node_set_mul(self, value)
        self._mul = value

    @property
    def add(self):
        return self._add

    @add.setter
    def add(self, value):
        node_set_add(self, value)
        self._add = value


class Sin(Node):
    def __init__(self, freq=100, mul=1, add=0):
        init_freq = 100 if isinstance(freq, Node) else freq
        super().__init__([lib.sin_node(ctypes.c_double(init_freq))], mul, add)
        self.freq = freq

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, value):
        node_set(self, 0, value)
        self._freq = value


class SqDetune(Node):
    def __init__(self, freq=100, mul=1, add=0):
        super().__init__([lib.sq_detune_node(ctypes.c_double(freq))], mul, add)
        self.freq = freq

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, value):
        node_set(self, 0, value)
        self._freq = value


class Container(Node):
    def __init__(self, head, mappings, mul=1, add=0):
        super().__init__([lib.container_node(head.ptr)], mul, add)
        self.head = head
        self.mappings = {}
        for i, m in enumerate(mappings):
            self.mappings[m] = i

    def embed_in_graph(self):
        lib.ctx_add_after_tail(self.ptr)

    def set(self, _idx, val):
        idx = _idx
        if isinstance(idx, str):
            idx = self.mappings.get(idx)
            # if not idx:
            #     raise Exception(f"{self} has no mapping for {_idx}")

        node_set(self, idx, val)


def start():
    lib.setup_audio()


def sin(freq=100):
    osc = Sin(freq)
    lib.ctx_add_after_tail(osc.container())
    return osc


def sq_detune(freq=100):
    osc = SqDetune(freq)
    container = Container(osc, ['freq'])
    container.embed_in_graph()
    return container


def sq_detune_delay(freq=100):
    osc = SqDetune(freq)
    container = Container(osc, ['freq'])
    container.embed_in_graph()
    return container


def play_seq(func, args):
    thread = threading.Thread(target=func, args=args)
    thread.start()
    return thread


def dump_nodes():
    lib.dump_graph()


start()
