.PHONY: all build buildnrun run clean lsp
PROJECT = PPMViewer.xcodeproj
SCHEME = PPMViewer
BUILD_DIR = build

# The path to the binary inside the Mac App Bundle
BINARY_PATH = $(BUILD_DIR)/Build/Products/Debug/$(SCHEME).app/Contents/MacOS/$(SCHEME)

# --- TARGETS ---

# Default target (what happens if you just type 'make')
all: buildnrun

# 1. Build the project using Xcode
build:
	xcodebuild -project $(PROJECT) \
		-scheme $(SCHEME) \
		-configuration Debug \
		-derivedDataPath $(BUILD_DIR) \
		-quiet build

# 2. Run the app (depends on build finishing successfully)
buildnrun: build
	@echo "-----------------------------------"
	@echo "Starting Application..."
	@echo "-----------------------------------"
	@$(BINARY_PATH)

# 3. Run the app without building it
run:
	@echo "-----------------------------------"
	@echo "Starting Application..."
	@echo "-----------------------------------"
	@$(BINARY_PATH)

# 4. Clean up build artifacts
clean:
	rm -rf $(BUILD_DIR)

# 5. Generate compile_commands.json for Doom/LSP
lsp:
	xcodebuild -project $(PROJECT) \
		-scheme $(SCHEME) \
		-configuration Debug \
		clean build \
		CODE_SIGNING_ALLOWED=NO \
		| xcpretty -r json-compilation-database -o compile_commands.json
