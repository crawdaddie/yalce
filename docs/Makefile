.PHONY: build serve watch dev

build:
	./build.py

serve:
	python3 -m http.server 8000 -d web

watch:
	@echo "Watching docs/ for changes... (Press Ctrl+C to stop)"
	@trap 'echo "\nStopping watch..."; exit 0' INT; \
	while true; do \
		fswatch -1 -e "web/" -e ".*\.html" .; \
		echo "Change detected, rebuilding..."; \
		./build.py; \
	done

dev:
	@echo "Starting development mode..."
	@echo "Building initial docs..."
	@./build.py
	@echo "Starting server at http://localhost:8000"
	@echo "Press Ctrl+C to stop"
	@trap 'echo "\nStopping dev server..."; kill 0' INT; \
	(python3 -m http.server 8000 -d web > /dev/null 2>&1) & \
	while true; do \
		fswatch -1 -e "web/" .; \
		echo "Change detected, rebuilding..."; \
		./build.py; \
	done

