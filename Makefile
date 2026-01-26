.PHONY: all build buildnrun run clean lsp
PROJECT = PPMViewer.xcodeproj
SCHEME = PPMViewer
BUILD_DIR = build

BINARY_PATH = $(BUILD_DIR)/Build/Products/Debug/$(SCHEME).app/Contents/MacOS/$(SCHEME)

# TARGETS
all: buildnrun

build:
	xcodebuild -project $(PROJECT) \
		-scheme $(SCHEME) \
		-configuration Debug \
		-derivedDataPath $(BUILD_DIR) \
		-quiet build

run:
	@echo "-----------------------------------"
	@echo "Starting Application..."
	@echo "-----------------------------------"
	@$(BINARY_PATH)

buildnrun: build run

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
