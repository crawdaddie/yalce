from ipykernel.kernelbase import Kernel
import subprocess
import tempfile
import os
import sys
from threading import Thread
from queue import Queue, Empty

def enqueue_output(out, queue):
    for line in iter(out.readline, b''):
        queue.put(line)
    out.close()

class YLCKernel(Kernel):
    implementation = "YLC"
    implementation_version = "1.0"
    language = "ylc"
    language_version = "0.1"
    language_info = {
        "name": "ylc",
        "mimetype": "text/plain",
        "file_extension": ".ylc",
    }
    banner = "YLC Language Kernel"
    
    def __init__(self, **kwargs):
        super(YLCKernel, self).__init__(**kwargs)
        self.ylc_process = subprocess.Popen(
            ["ylc", "-i"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            bufsize=1,
            universal_newlines=True
        )
        
        # Set up output queues and threads
        self.stdout_queue = Queue()
        self.stderr_queue = Queue()
        
        self.stdout_thread = Thread(
            target=enqueue_output, 
            args=(self.ylc_process.stdout, self.stdout_queue)
        )
        self.stdout_thread.daemon = True
        self.stdout_thread.start()
        
        self.stderr_thread = Thread(
            target=enqueue_output, 
            args=(self.ylc_process.stderr, self.stderr_queue)
        )
        self.stderr_thread.daemon = True
        self.stderr_thread.start()
        
        # Read initial output (welcome message)
        self._read_output()

    def _read_output(self, timeout=0.1):
        stdout_content = []
        stderr_content = []
        
        try:
            while True:
                line = self.stdout_queue.get_nowait()
                stdout_content.append(line)
        except Empty:
            pass
            
        try:
            while True:
                line = self.stderr_queue.get_nowait()
                stderr_content.append(line)
        except Empty:
            pass
            
        return "".join(stdout_content), "".join(stderr_content)

    def do_execute(
        self, code, silent, store_history=True, user_expressions=None, allow_stdin=False
    ):
        try:
            self.ylc_process.stdin.write(code + "\n")
            self.ylc_process.stdin.flush()
            
            import time
            time.sleep(0.05)
            
            stdout, stderr = self._read_output()
            
            if not silent:
                if stdout:
                    stream_content = {"name": "stdout", "text": stdout}
                    self.send_response(self.iopub_socket, "stream", stream_content)
                
                if stderr:
                    stream_content = {"name": "stderr", "text": stderr}
                    self.send_response(self.iopub_socket, "stream", stream_content)
            
            return {
                "status": "ok",
                "execution_count": self.execution_count,
                "payload": [],
                "user_expressions": {},
            }
            
        except Exception as e:
            if not silent:
                error_content = {
                    "ename": type(e).__name__,
                    "evalue": str(e),
                    "traceback": ["Error sending code to YLC interpreter"],
                }
                self.send_response(self.iopub_socket, "error", error_content)
                
            return {
                "status": "error",
                "execution_count": self.execution_count,
                "ename": type(e).__name__,
                "evalue": str(e),
                "traceback": ["Error sending code to YLC interpreter"],
            }
    
    def do_shutdown(self, restart):
        """Shut down the YLC process"""
        self.ylc_process.terminate()
        try:
            self.ylc_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.ylc_process.kill()
