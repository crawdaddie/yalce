import os
import subprocess
import sys
import tempfile
from queue import Empty, Queue
from threading import Thread

from ipykernel.kernelbase import Kernel


def enqueue_output(out, queue):
    for line in iter(out.readline, b""):
        queue.put(line)
    out.close()


def escape_text_ylc(text):
    lines = text.split("\n")

    if len(lines) <= 1:
        return text + "\n\n"

    result = []

    for i in range(len(lines) - 1):
        line = lines[i]
        if any(not c.isspace() for c in line):
            result.append(line + " \\")

    if lines[-1] and any(not c.isspace() for c in lines[-1]):
        result.append(lines[-1])

    return "\n".join(result) + "\n\n"


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
            universal_newlines=True,
        )

        # Set up output queues and threads
        self.stdout_queue = Queue()
        self.stderr_queue = Queue()

        self.stdout_thread = Thread(
            target=enqueue_output, args=(self.ylc_process.stdout, self.stdout_queue)
        )
        self.stdout_thread.daemon = True
        self.stdout_thread.start()

        self.stderr_thread = Thread(
            target=enqueue_output, args=(self.ylc_process.stderr, self.stderr_queue)
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

    def _process_output(self, stdout, stderr):
        """Process output and detect special display data"""

        if stdout.startswith("%display_plot:"):
            plot_data = stdout.replace("%display_plot:", "").strip()

            if plot_data.startswith("<svg"):
                self.send_response(
                    self.iopub_socket,
                    "display_data",
                    {"data": {"image/svg+xml": plot_data}, "metadata": {}},
                )
                return True

            if plot_data.startswith("data:image/png;base64,"):
                png_data = plot_data.replace("data:image/png;base64,", "")
                self.send_response(
                    self.iopub_socket,
                    "display_data",
                    {"data": {"image/png": png_data}, "metadata": {}},
                )
                return True

        return False

    def do_execute(
        self, code, silent, store_history=True, user_expressions=None, allow_stdin=False
    ):
        try:
            self.ylc_process.stdin.write(escape_text_ylc(code))
            self.ylc_process.stdin.flush()

            import time

            time.sleep(0.05)

            stdout, stderr = self._read_output()

            if not silent:
                if not self._process_output(stdout, stderr):
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
